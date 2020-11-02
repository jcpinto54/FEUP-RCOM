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
extern applicationLayer application;

extern int idFrameSent;
extern frame_t *lastFrameReceived;


int llopen(char *port, int appStatus)
{
    struct termios oldtio, newtio;

    application.fd = open(port, O_RDWR | O_NOCTTY);
    if (application.fd < 0) {
        perror(port);
        return -1;
    }

    if (tcgetattr(application.fd, &oldtio) == -1) {
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
    newtio.c_cc[VMIN] = 0;  // min number of chars to read

    if (tcsetattr(application.fd, TCSANOW, &newtio) == -1) {
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
                    printf("Max Number of retransmissions reached. Exiting program.\n");
                    return -1;
                }

                buildSETFrame(&setFrame, true);
                if (sendNotIFrame(&setFrame)) return -5;

                prepareToReceive(&responseFrame, 5);
                int responseReceive = receiveNotIMessage(&responseFrame, RESPONSE_WITHOUT_ID); 
                if (responseReceive == -1 || responseReceive == -2) continue;         // in a timeout or wrong bcc, retransmit frame
                else if (responseReceive < -2) return -7;
                if (!isUAFrame(&responseFrame)) continue;       // wrong frame received

                break;
            }
            break;
        case RECEIVER:;
            prepareToReceive(&receiverFrame, 5);
            if (receiveNotIMessage(&receiverFrame, RESPONSE_WITHOUT_ID)) return -7;
            if (!isSETFrame(&receiverFrame)) return -8;

            buildUAFrame(&uaFrame, true);
            if (sendNotIFrame(&uaFrame)) return -5;
            
            break;
    }

    printf("LLOPEN DONE\n");
    return application.fd;
}

int llclose(int fd) {

    frame_t discFrame;
    frame_t receiveFrame;
    frame_t uaFrame;

    int receiveReturn;
    switch (application.status) {
        case TRANSMITTER:;
            for (int i = 0;; i++) {
                if (i == MAX_FRAME_RETRANSMISSIONS) {
                    printf("Max Number of retransmissions reached. Exiting program.\n");
                    return -1;
                }

                buildDISCFrame(&discFrame, true);
                if (sendNotIFrame(&discFrame)) return -2;

                prepareToReceive(&receiveFrame, 5);
                receiveReturn = receiveNotIMessage(&receiveFrame, RESPONSE_WITHOUT_ID);
                if (receiveReturn == -1 || receiveReturn == -2) continue;        //in a timeout or wrong bcc, retransmit frame
                else if (receiveReturn < -2) return -4;
                if (!isDISCFrame(&receiveFrame)) continue;      // wrong frame received

                buildUAFrame(&uaFrame, true);
                if (sendNotIFrame(&uaFrame)) return -2;
                
                break;
            }
        break;
        case RECEIVER:;
            for (int i = 0; i < MAX_READ_ATTEMPTS; i++) {
                prepareToReceive(&receiveFrame, 5);
                int receiveReturn = receiveNotIMessage(&receiveFrame, RESPONSE_WITHOUT_ID);
                if (receiveReturn == -1) continue;
                else if (receiveReturn) return -7;
                if (!isDISCFrame(&receiveFrame)) return -5;
                break;
            }
            buildDISCFrame(&discFrame, true);
            if (sendNotIFrame(&discFrame)) return -2;

            prepareToReceive(&receiveFrame, 5);
            int receiveNotIMessageReturn = receiveNotIMessage(&receiveFrame, RESPONSE_WITHOUT_ID);
            if (receiveNotIMessageReturn) return -4;
            if (!isUAFrame(&receiveFrame)) return -5;

        break;
    }
    if (close(fd) == -1) return -8;
    printf("LLCLOSE DONE\n");
    return 1;
}


int llread(int fd, char ** buffer){
    frame_t frame, response;
    int receiveIMessageReturn, bufferLength = MAX_FRAME_DATA_LENGTH;
    *buffer = (char *)malloc(MAX_FRAME_DATA_LENGTH);
    do {
        receiveIMessageReturn = receiveIMessage(&frame);
        if (receiveIMessageReturn < -4 || receiveIMessageReturn > 3) {
            printf("receiveIMessage returned unexpected value\n");
            return -1;
        }
        
        if (receiveIMessageReturn >= 0 && receiveIMessageReturn <= 3) {
            prepareResponse(&response, true, (frame.infoId + 1) % 2);
            printf("Sent RR frame to the transmitter\n");
        }
        else if (receiveIMessageReturn == -2 && receiveIMessageReturn == -3) {
            if (frame.infoId == lastFrameReceived->infoId) {
                prepareResponse(&response, true, (frame.infoId + 1) % 2);
                printf("Read a duplicate frame\nSent RR frame to the transmitter\n");
            }
            else { 
                prepareResponse(&response, false, (frame.infoId + 1) % 2);
                printf("Sent REJ frame to the transmitter\n");
            }
        }
        if (receiveIMessageReturn != -1 || receiveIMessageReturn != -4) {
            if (sendNotIFrame(&response) == -1) return -1;
        }
        
        if (receiveIMessageReturn == 0 || receiveIMessageReturn == 1)
            memcpy(*buffer + bufferLength - MAX_FRAME_DATA_LENGTH, frame.bytes + 5, frame.bytes[4]);        

        if (receiveIMessageReturn == 1) {
            *buffer = (char *)realloc(*buffer, bufferLength);
            bufferLength += MAX_FRAME_DATA_LENGTH;
        }


        if (receiveIMessageReturn == -4) {
            printf("Serial Port couldn't be read. Exiting llread...\n");
            return -1;
        }
        else if (receiveIMessageReturn == -1)
            printf("No action was done\n");



    } while (receiveIMessageReturn != 0 && receiveIMessageReturn != 2);

    return 0;
}

int llwrite(int fd, char * buffer, int length)
{
    frame_t **info = NULL;
    int framesToSend = prepareI(buffer, length, &info); //Prepara a trama de informação
    printf("Divided the data in %d frames. Sending all frames...\n", framesToSend);
    for (int i = 0; i < framesToSend; i++) {
        if (sendIFrame(info[i]) == -1) return -1;
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
    while (read(auxFd, &c, 1) != 0) printf("byte cleared: %x\n", c);
    if (close(auxFd) == -1) return 2;
    return 0;
}
