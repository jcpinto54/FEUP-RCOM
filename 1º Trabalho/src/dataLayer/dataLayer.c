#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "dataLayer.h"
#include "dataLayerPrivate.h"

int status;
extern int idFrameSent;
extern int lastFrameReceivedId;

int llopen(char *port, int appStatus)
{
    status = appStatus;

    struct termios oldtio, newtio;

    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(port);
        return -1;
    }

    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        return -2;
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; // time to time-out in deciseconds
    newtio.c_cc[VMIN] = 1;  // min number of chars to read

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return -3;
    }

    frame_t setFrame;
    frame_t responseFrame;
    frame_t receiverFrame;
    frame_t uaFrame;

    switch (appStatus) {
        case TRANSMITTER:;
            for (int i = 0;; i++) {
                if (i == MAX_FRAME_RETRANSMISSIONS) {
                    printf("DATA - Max Number of retransmissions reached. Exiting program.\n");
                    return -1;
                }

                buildSETFrame(&setFrame, true);
                
                printf("setframe size: %lu\n", setFrame.size);
                printf("setframe flag: %x\n", setFrame.bytes[0] & 0xff);
                printf("setframe a: %x\n", setFrame.bytes[1]& 0xff);
                printf("setframe c: %x\n", setFrame.bytes[2]& 0xff);
                printf("setframe bcc: %x\n", setFrame.bytes[3]& 0xff);
                printf("setframe flag: %x\n\n\n", setFrame.bytes[4]& 0xff);
                
                
                if (sendNotIFrame(&setFrame, fd)) {
                    perror("sendNotIFrame\n");
                    return -5;
                }
                prepareToReceive(&responseFrame, 5);
                printf("in receive");
                int responseReceive = receiveNotIMessage(&responseFrame, fd, RESPONSE_WITHOUT_ID, 3); 
                printf("out receive");
                
                printf("response size: %lu\n", responseFrame.size);
                printf("response flag: %x\n", responseFrame.bytes[0] & 0xff);
                printf("response a: %x\n", responseFrame.bytes[1]& 0xff);
                printf("response c: %x\n", responseFrame.bytes[2]& 0xff);
                printf("response bcc: %x\n", responseFrame.bytes[3]& 0xff);
                printf("response flag: %x\n\n\n", responseFrame.bytes[4]& 0xff);
                

                if (responseReceive == -1) continue;         // in a timeout, retransmit frame
                else if (responseReceive < -2) {perror("responseReceive\n");  return -7;}
                if (!isUAFrame(&responseFrame)) continue;       // wrong frame received

                break;
            }
            break;
        case RECEIVER:;
            prepareToReceive(&receiverFrame, 5);
            int error = receiveNotIMessage(&receiverFrame, fd, RESPONSE_WITHOUT_ID, 3);
            if (error) {printf("receiveNotIMessage returned %d\n", error); return -7;}
            if (!isSETFrame(&receiverFrame)) {perror("isSETFrame\n"); return -8;}

            buildUAFrame(&uaFrame, true);
            if (sendNotIFrame(&uaFrame, fd)) {perror("sendNotIFrame\n"); return -5;}
            
            break;
    }
    printf("DATA - Opened serial port connection\n");
    return fd;
}

int llclose(int fd) {

    frame_t discFrame;
    frame_t receiveFrame;
    frame_t uaFrame;

    int receiveReturn;
    switch (status) {
        case TRANSMITTER:;
            for (int i = 0;; i++) {
                if (i == MAX_FRAME_RETRANSMISSIONS) {
                    printf("DATA - Max Number of retransmissions reached. Exiting program.\n");
                    return -1;
                }

                buildDISCFrame(&discFrame, true);
                if (sendNotIFrame(&discFrame, fd)) return -2;

                prepareToReceive(&receiveFrame, 5);
                receiveReturn = receiveNotIMessage(&receiveFrame, fd, RESPONSE_WITHOUT_ID, 7);
                if (receiveReturn == -1) continue;        //in a timeout, retransmit frame
                else if (receiveReturn < -1) return -4;
                if (!isDISCFrame(&receiveFrame)) continue;      // wrong frame received

                buildUAFrame(&uaFrame, true);
                if (sendNotIFrame(&uaFrame, fd)) return -2;
                
                break;
            }
        break;
        case RECEIVER:;
            for (int i = 0; i < MAX_READ_ATTEMPTS; i++) {
                prepareToReceive(&receiveFrame, 5);
                int receiveReturn = receiveNotIMessage(&receiveFrame, fd, RESPONSE_WITHOUT_ID, 7);
                if (receiveReturn == -1) continue;
                else if (receiveReturn) return -7;
                if (!isDISCFrame(&receiveFrame)) return -5;
                break;
            }
            buildDISCFrame(&discFrame, true);
            if (sendNotIFrame(&discFrame, fd)) return -2;

            prepareToReceive(&receiveFrame, 5);
            int receiveNotIMessageReturn = receiveNotIMessage(&receiveFrame, fd, RESPONSE_WITHOUT_ID, 3);
            if (receiveNotIMessageReturn) return -4;
            if (!isUAFrame(&receiveFrame)) return -5;

        break;
    }
    if (close(fd) == -1) return -8;
    printf("DATA - Closed serial port connection\n");
    return 1;
}


int llread(int fd, char ** buffer){
    frame_t frame, response;
    int receiveIMessageReturn, bufferLength = MAX_FRAME_DATA_LENGTH, sameReadAttempts = 1;
    *buffer = (char *)malloc(MAX_FRAME_DATA_LENGTH);
    do {
        receiveIMessageReturn = receiveIMessage(&frame, fd, 3);
        
        if (receiveIMessageReturn == -5) {
            printf("DATA - Read timeout. Exiting llread...\n");
            return -1;
        }
        
        if (receiveIMessageReturn < -5 || receiveIMessageReturn > 3) {
            printf("DATA - receiveIMessage returned unexpected value\n");
            return -1;
        }
        
        if (receiveIMessageReturn >= 0 && receiveIMessageReturn <= 3) {
            prepareResponse(&response, true, (frame.infoId + 1) % 2);
            printf("DATA - Sent RR frame to the transmitter\n");
            sameReadAttempts = 0;
        }
        else if (receiveIMessageReturn == -2 || receiveIMessageReturn == -3) {
            if (lastFrameReceivedId != -1 && frame.infoId == lastFrameReceivedId) {
                prepareResponse(&response, true, (frame.infoId + 1) % 2);
                printf("DATA - Read a duplicate frame\nDATA - Sent RR frame to the transmitter\n");
                sameReadAttempts = 0;
            }
            else { 
                prepareResponse(&response, false, (frame.infoId + 1) % 2);
                printf("DATA - Sent REJ frame to the transmitter\n");
                sameReadAttempts++;
            }
        }
        if (receiveIMessageReturn >= 0) {
            lastFrameReceivedId = frame.infoId;
        }
        if (receiveIMessageReturn != -1 || receiveIMessageReturn != -4) {
            if (sendNotIFrame(&response, fd) == -1) return -1;
        }
        
        if (receiveIMessageReturn == 0 || receiveIMessageReturn == 1)
            memcpy(*buffer + bufferLength - MAX_FRAME_DATA_LENGTH, frame.bytes + 5, frame.bytes[4]);        

        if (receiveIMessageReturn == 1) {
            *buffer = (char *)realloc(*buffer, bufferLength);
            bufferLength += MAX_FRAME_DATA_LENGTH;
        }


        if (receiveIMessageReturn == -4) {
            printf("DATA - Serial Port couldn't be read. Exiting llread...\n");
            return -1;
        }
        else if (receiveIMessageReturn == -1)
            printf("DATA - No action was done\n");

    } while (receiveIMessageReturn != 0 && receiveIMessageReturn != 2 && sameReadAttempts < MAX_READ_ATTEMPTS);

    if (sameReadAttempts == MAX_READ_ATTEMPTS) {
        printf("DATA - Max read attempts of the same frame reached.\n");
        return -1;
    }

    return 0;
}

int llwrite(int fd, char * buffer, int length)
{
    frame_t **info = NULL;
    int framesToSend = prepareI(buffer, length, &info); //Prepara a trama de informação

    printf("DATA - Divided the data in %d frames. Sending all frames...\n", framesToSend);
    for (int i = 0; i < framesToSend; i++) {
        printFrame(info[i]);
        if (sendIFrame(info[i], fd) == -1) return -1;
    }
    return 0;
}


int clearSerialPort(char *port) {
    int auxFd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (auxFd == -1) {
        perror("error clearing serialPort");
        return 1;
    }
    char c;
    while (read(auxFd, &c, 1) != 0) printf("DATA - byte cleared: %x\n", c);
    if (close(auxFd) == -1) return 2;
    return 0;
}
