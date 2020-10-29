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
#include "macros.h"

extern applicationLayer app;

int llopen(char *port, int appStatus)
{
    printf("ENTERED LLOPEN\n");
    
    struct termios oldtio, newtio;

    app.fd = open(port, O_RDWR | O_NOCTTY);
    if (app.fd < 0) {
        perror(port);
        return -1;
    }

    if (tcgetattr(app.fd, &oldtio) == -1) {
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

    if (tcsetattr(app.fd, TCSANOW, &newtio) == -1) {
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
                    exit(1);
                }

                if (buildSETFrame(&setFrame, true)) return -4;
                if (sendMessage(setFrame)) return -5;
                destroyFrame(&setFrame);

                if (prepareToReceive(&responseFrame, 5)) return -6;
                int responseReceive = receiveNotIMessage(&responseFrame, true); 
                if (responseReceive == 1 || responseReceive == 2) continue;         // in a timeout or wrong bcc, retransmit frame
                else if (responseReceive > 2) return -7;
                if (!isUAFrame(&responseFrame)) continue;       // wrong frame received
                destroyFrame(&responseFrame);

                printf("Done, Transmitter Ready\n");
                break;
            }
            break;
        case RECEIVER:;
            if (prepareToReceive(&receiverFrame, 5)) return -6;
            if (receiveNotIMessage(&receiverFrame, true)) return -7;
            if (!isSETFrame(&receiverFrame)) return -8;
            destroyFrame(&receiverFrame);

            if (buildUAFrame(&uaFrame, true)) return -4;
            if (sendMessage(uaFrame)) return -5;
            destroyFrame(&uaFrame);
            
            printf("Done, Receiver Ready\n");
            break;
    }
    return app.fd;
}

int llclose(int fd) {
    printf("ENTERED LLCLOSE\n");

    frame_t discFrame;
    frame_t receiveFrame;
    frame_t uaFrame;

    int receiveReturn;
    switch (app.status) {
        case TRANSMITTER:;
            for (int i = 0;; i++) {
                if (i == MAX_FRAME_RETRANSMISSIONS) {
                    printf("Max Number of retransmissions reached. Exiting program.\n");
                    exit(1);
                }

                if (buildDISCFrame(&discFrame, true)) return -1;
                if (sendMessage(discFrame)) return -2;
                destroyFrame(&discFrame);

                if (prepareToReceive(&receiveFrame, 5)) return -3;
                receiveReturn = receiveNotIMessage(&receiveFrame, true);
                if (receiveReturn == 1 || receiveReturn == 2) continue;        //in a timeout or wrong bcc, retransmit frame
                else if (receiveReturn > 2) return -4;
                if (!isDISCFrame(&receiveFrame)) continue;      // wrong frame received
                destroyFrame(&receiveFrame);

                if (buildUAFrame(&uaFrame, true)) return -1;
                if (sendMessage(uaFrame)) return -2;
                destroyFrame(&uaFrame);
                
                printf("Done, Transmitter Out\n");
                break;
            }
        break;
        case RECEIVER:;
            if (prepareToReceive(&receiveFrame, 5)) return -3;
            if (receiveNotIMessage(&receiveFrame, true)) return -7;
            if (!isDISCFrame(&receiveFrame)) return -5;
            destroyFrame(&receiveFrame);

            if (buildDISCFrame(&discFrame, true)) return -1;
            if (sendMessage(discFrame)) return -2;
            destroyFrame(&discFrame);

            if (prepareToReceive(&receiveFrame, 5)) return -3;
            if (receiveNotIMessage(&receiveFrame, true)) return -4;
            if (!isUAFrame(&receiveFrame)) return -5;
            destroyFrame(&receiveFrame);

            printf("Done, Receiver Out\n");
        break;
    }
    if (close(fd) == -1) return -8;
    return 1;
}

// ---

int llread(int fd, char * buffer){
    frame_t frame;
    bool response;
    receiveIMessage(&frame);
    checkIMessage(&frame);
    sendResponse(response);

}

int receiveIMessage(frame_t *frame){
    u_int8_t c;
    receive_state_t state = INIT;
    time_t initTime, curTime;
    int dataCounter = 0;
    initTime = time(NULL);
    do {
        int bytesRead = read(app.fd, &c, 1);
        if (bytesRead < 0) {
            perror("read error");
            return 3;
        }

        switch (state) {
            case INIT:
                if (c == FLAG) {
                    state = RCV_FLAG;
                    frame->bytes[0] = c;
                }
                break;
            case RCV_FLAG:
                if (c == TRANSMITTER_TO_RECEIVER || c == RECEIVER_TO_TRANSMITTER) {
                    state = RCV_A;
                    frame->bytes[1] = c;
                }
                else
                    state = INIT;
                break;
            case RCV_A:
                if (c == SET || c == UA || c == DISC || c == RR || c == REJ) {
                    state = RCV_C;
                    frame->bytes[2] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                else
                    state = INIT;
                break;
            case RCV_C:
                if (bccVerifier(frame->bytes, 1, 2, c)) {
                    state = RCV_BCC1;
                    frame->bytes[3] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                else {
                    perror("BCC1 not correct\n");
                    return 2;
                }
                break;
            case RCV_BCC1:
                if (c == 0x77) {
                    state = RCV_DATA;
                    frame->bytes[4] = c;
                }
                else
                    state = INIT;
                break;

            case RCV_DATA:
                if(!dataCounter){
                    prepareToReceiveData(&frame, (size_t) c);
                }
                addReceiveData(&frame, c, dataCounter);
                dataCounter++;
                if(dataCounter == frame->data[0]) state = RCV_BCC2;
                break;
            case RCV_BCC2:
                if(frame->data[3] == c) state = COMPLETE;
                perror("BCC1 and BCC2 don't match. Exiting...\n");
                break;

            case COMPLETE:
                break;
        }
    } while (state != COMPLETE);
    
    return 0;
}

int receiveNotIMessage(frame_t *frame, bool isResponse)
{
    u_int8_t c;
    receive_state_t state = INIT;
    time_t initTime, curTime;
    initTime = time(NULL);
    do {
        int bytesRead = read(app.fd, &c, 1);
        if (bytesRead < 0) {
            perror("read error");
            return 3;
        }
        else if (bytesRead == 0 && isResponse) {
            curTime = time(NULL);
            time_t seconds = curTime - initTime;
            if (seconds >= 3.0) {
                printf("Read Timeout!\n");
                return 1;
            }
        }
        else if (bytesRead > 0 && isResponse) {
            initTime = time(NULL);
        }

        switch (state) {
            case INIT:
                if (c == FLAG) {
                    state = RCV_FLAG;
                    frame->bytes[0] = c;
                }
                break;
            case RCV_FLAG:
                if (c == TRANSMITTER_TO_RECEIVER || c == RECEIVER_TO_TRANSMITTER) {
                    state = RCV_A;
                    frame->bytes[1] = c;
                }
                else
                    state = INIT;
                break;
            case RCV_A:
                if (c == SET || c == UA || c == DISC || c == RR || c == REJ) {
                    state = RCV_C;
                    frame->bytes[2] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                else
                    state = INIT;
                break;
            case RCV_C:
                if (bccVerifier(frame->bytes, 1, 2, c)) {
                    state = RCV_BCC1;
                    frame->bytes[3] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                else {
                    perror("BCC1 not correct\n");
                    return 2;
                }
                break;
            case RCV_BCC1:
                if (c == FLAG) {
                    state = COMPLETE;
                    frame->bytes[4] = c;
                }
                else
                    state = INIT;
                break;
            case COMPLETE:
                break;
        }
    } while (state != COMPLETE);
    
    return 0;
}

int sendMessage(frame_t frame) {
    int sentBytes = 0;

    for (int attempts = 0;(sentBytes != frame.size); attempts++) {
        if (attempts >= MAX_WRITE_ATTEMPTS) {
            perror("Too many failed attempts to send. Time out!\n");
            return 1;
        }
        sentBytes += write(app.fd, frame.bytes, frame.size);
        printf("%d bytes sent\n", sentBytes);
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

// ---

u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length)
{
    int onesCounter = 0;
    for (int i = start; i < start + length; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            onesCounter += getBit(bytes[i], j);
        }
    }
    return onesCounter % 2;
}

bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity)
{
    if (bccCalculator(bytes, start, length) == parity)
        return true;
    return false;
}

int buildSETFrame(frame_t *frame, bool transmitterToReceiver)
{
    if ((frame->bytes = malloc(5)) == NULL) return 1;
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = SET;
    frame->bytes[3] = 0;    // BCC
    frame->bytes[4] = FLAG;

    return 0;
}

bool isSETFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == SET;
}

int buildUAFrame(frame_t *frame, bool transmitterToReceiver)
{
    if ((frame->bytes = malloc(5)) == NULL) return 1;
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = UA;
    frame->bytes[3] = 1;    // BCC
    frame->bytes[4] = FLAG;

    return 0;
}

bool isUAFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == UA;
}

int buildDISCFrame(frame_t *frame, bool transmitterToReceiver)
{
    if ((frame->bytes = malloc(5)) == NULL) return 1;
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = DISC;
    frame->bytes[3] = 1;    // BCC
    frame->bytes[4] = FLAG;

    return 0;
}

bool isDISCFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == DISC;
}

void destroyFrame(frame_t *frame)
{
    free(frame->bytes);
    if(frame->data != NULL) free(frame->data);
}

int prepareToReceive(frame_t *frame, size_t size)
{
    frame->size = size;
    return (frame->bytes = malloc(frame->size)) == NULL;
}

int prepareToReceiveData(frame_t *frame, size_t size){
    return (frame->data = malloc(size)) == NULL;
}

int addReceiveData(frame_t * frame, char data, int local){
    frame->data[local] = data;
}