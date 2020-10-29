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

                buildSETFrame(&setFrame, true);
                if (sendMessage(setFrame)) return -5;

                prepareToReceive(&responseFrame, 5);
                int responseReceive = receiveNotIMessage(&responseFrame); 
                if (responseReceive == 1 || responseReceive == 2) continue;         // in a timeout or wrong bcc, retransmit frame
                else if (responseReceive > 2) return -7;
                if (!isUAFrame(&responseFrame)) continue;       // wrong frame received

                printf("Done, Transmitter Ready\n");
                break;
            }
            break;
        case RECEIVER:;
            prepareToReceive(&receiverFrame, 5);
            if (receiveNotIMessage(&receiverFrame)) return -7;
            if (!isSETFrame(&receiverFrame)) return -8;

            buildUAFrame(&uaFrame, true);
            if (sendMessage(uaFrame)) return -5;
            
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

                buildDISCFrame(&discFrame, true);
                if (sendMessage(discFrame)) return -2;

                prepareToReceive(&receiveFrame, 5);
                receiveReturn = receiveNotIMessage(&receiveFrame);
                if (receiveReturn == 1 || receiveReturn == 2) continue;        //in a timeout or wrong bcc, retransmit frame
                else if (receiveReturn > 2) return -4;
                if (!isDISCFrame(&receiveFrame)) continue;      // wrong frame received

                buildUAFrame(&uaFrame, true);
                if (sendMessage(uaFrame)) return -2;
                
                printf("Done, Transmitter Out\n");
                break;
            }
        break;
        case RECEIVER:;
            prepareToReceive(&receiveFrame, 5);
            if (receiveNotIMessage(&receiveFrame)) return -7;
            if (!isDISCFrame(&receiveFrame)) return -5;

            buildDISCFrame(&discFrame, true);
            if (sendMessage(discFrame)) return -2;

            prepareToReceive(&receiveFrame, 5);
            if (receiveNotIMessage(&receiveFrame)) return -4;
            if (!isUAFrame(&receiveFrame)) return -5;

            printf("Done, Receiver Out\n");
        break;
    }
    if (close(fd) == -1) return -8;
    return 1;
}

int llread(int fd, char * buffer){
    frame_t frame;
    //bool response;
    int size;
    if ((size = receiveIMessage(&frame)) < 0) {
        perror("Error in receiveIMessage");
        return -1;
    }
    // checkIMessage(&frame);
    // sendResponse(response);

    return size;
}

int llwrite(int fd, char * buffer, int length)
{
    // frame_t **info = NULL, *responseFrame = NULL;
    // int framesToSend = prepareI(buffer, length, info); //Prepara a trama de informação
    // bool stop = false;
    // int attempts = 0, result = -1;
    
    // do
    // {
    //     if(attempts >= MAX_WRITE_ATTEMPTS) 
    //     {
    //         perror("ERROR: Too many write attempts\n");
    //         stop = true;
    //     }

    //     // sendMessage(*info);                  
    //     receiveNotIMessage(responseFrame);

    //     if(responseFrame->bytes[2] == RR) 
    //     {
    //         // result = info->size;
    //         stop = true;
    //     }
    //     else if(responseFrame->bytes[2] == REJ)
    //     {
    //         attempts++;
    //     }
    //     else
    //     {
    //         perror("ERROR: Unknown response frame");
    //     }
    // }while(!stop);

    // destroyFrame(info);
    // destroyFrame(responseFrame);
    
    // return result;
    return 0;
}

//---

void auxStuffing(frame_t * frame, int * stuffingCounter, char *data)
{
    for(unsigned int i = 0; i < frame->bytes[4]; i++) {
        if(data[i] == FLAG){//do byte stuffing
            frame->bytes[5 + i + (*stuffingCounter)] = ESC;
            frame->bytes[5 + i + (++(*stuffingCounter))] = FLAG_STUFFING;
        }
        else if(data[i] == ESC){//do byte stuffing
            frame->bytes[5 + i + (*stuffingCounter)] = ESC;
            frame->bytes[5 + i + (++(*stuffingCounter))] = ESC_STUFFING;
        }
        else{
            frame->bytes[5 + i + (*stuffingCounter)] = data[i];
        }
    }
}

// pode ser necessário ter os dados em mais que uma frame
int prepareI(char* data, int length, frame_t *** infoNew) //Testar
{
    int framesNeeded = 1;
    if (length > MAX_FRAME_DATA_LENGTH) {
        framesNeeded = (length / MAX_FRAME_DATA_LENGTH) + 1;
    }

    u_int8_t frameDataSize;
    frame_t **info = malloc(sizeof(frame_t *) * framesNeeded);
    for (int i = 0; i < framesNeeded; i++) {
        printf("aaa\n");
        info[i] = malloc(sizeof(frame_t));

        info[i]->bytes[0] = FLAG; //F
        info[i]->bytes[1] = TRANSMITTER_TO_RECEIVER; //A
        info[i]->bytes[2] = I; //C: ID da trama, suposto mudar depois
        info[i]->bytes[3] = bccCalculator(info[i]->bytes, 1, 2); //BCC1, calculado com A e C
    
        int stuffingCounter = 0;
        //Talvez colocar o tamanho da mensagem como primeiro byte?

        unsigned lengthInOtherFrames = 0;
        if (i < framesNeeded - 1 || framesNeeded == 1) frameDataSize = length;
        else {
            for (int j = 0; j < i; j++) {
                lengthInOtherFrames += info[j]->bytes[4];
            }
            frameDataSize = length - lengthInOtherFrames;
        }
        info[i]->bytes[4] = frameDataSize;

        auxStuffing(info[i], &stuffingCounter, data + lengthInOtherFrames);

        int bcc2_byte_ix = 4 + 1 + frameDataSize + stuffingCounter;

        info[i]->bytes[bcc2_byte_ix] = bccCalculator(info[i]->bytes, 4, length);
        info[i]->bytes[bcc2_byte_ix + 1] = FLAG;
        info[i]->size = 7 + frameDataSize + stuffingCounter;
    }
    for (int j = 0; j < info[0]->size; j++) printf("i: %d   -   info[0]->bytes[i]: %x\n", j, info[0]->bytes[j]);
    *infoNew = info;
    return framesNeeded;
}


int receiveIMessage(frame_t *frame){
    // u_int8_t c;
    // receive_state_t state = INIT;
    // int dataCounter = 0;
    // do {
    //     int bytesRead = read(app.fd, &c, 1);
    //     if (bytesRead < 0) {
    //         perror("read error");
    //         return 3;
    //     }

    //     switch (state) {
    //         case INIT:
    //             if (c == FLAG) {
    //                 state = RCV_FLAG;
    //                 frame->bytes[0] = c;
    //             }
    //             break;
    //         case RCV_FLAG:
    //             if (c == TRANSMITTER_TO_RECEIVER || c == RECEIVER_TO_TRANSMITTER) {
    //                 state = RCV_A;
    //                 frame->bytes[1] = c;
    //             }
    //             else
    //                 state = INIT;
    //             break;
    //         case RCV_A:
    //             if (c == I) {
    //                 state = RCV_C;
    //                 frame->bytes[2] = c;
    //             }
    //             else if (c == FLAG)
    //                 state = RCV_FLAG;
    //             else
    //                 state = INIT;
    //             break;
    //         case RCV_C:
    //             if (bccVerifier(frame->bytes, 1, 2, c)) {
    //                 state = RCV_BCC1;
    //                 frame->bytes[3] = c;
    //             }
    //             else if (c == FLAG)
    //                 state = RCV_FLAG;
    //             else {
    //                 perror("BCC1 not correct\n");
    //                 return 2;
    //             }
    //             break;
    //         case RCV_BCC1:
    //             if (dataCounter == 0) {
    //                 prepareToReceiveData(frame, (size_t) c);
    //             }
    //             if (dataCounter == frame->dataSize) state = RCV_DATA;
    //             if (c == FLAG) {     
    //                 state = RCV_FLAG;
    //                 continue;
    //             }
    //             frame->data[dataCounter] = c;
    //             dataCounter++;
    //             break;
    //         case RCV_DATA:
    //             if (bccVerifier(frame->data, 0, frame->dataSize, c)) {
    //                 state = RCV_BCC2;
    //                 frame->bytes[3] = c;
    //             }
    //             else if (c == FLAG)
    //                 state = RCV_FLAG;
    //             else {
    //                 perror("BCC2 not correct\n");
    //                 return 2;
    //             }
    //             break;
    //         case RCV_BCC2:
    //             if (c == FLAG) {
    //                 state = COMPLETE;
    //                 frame->bytes[4] = c;
    //             }
    //             else
    //                 state = INIT;
    //             break;
    //         case COMPLETE: break;
    //     }
    // } while (state != COMPLETE);
    // destuffFrame(frame);
    return 0;
}

int receiveNotIMessage(frame_t *frame)
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
        else if (bytesRead == 0) {
            curTime = time(NULL);
            time_t seconds = curTime - initTime;
            if (seconds >= TIMEOUT) {
                printf("Read Timeout!\n");
                return 1;
            }
        }
        else if (bytesRead > 0) {
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
            default:
                perror("Unknown state");
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

void buildSETFrame(frame_t *frame, bool transmitterToReceiver)
{
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = SET;
    frame->bytes[3] = 0;    // BCC
    frame->bytes[4] = FLAG;
}

bool isSETFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == SET;
}

void buildUAFrame(frame_t *frame, bool transmitterToReceiver)
{
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = UA;
    frame->bytes[3] = 1;    // BCC
    frame->bytes[4] = FLAG;
}

bool isUAFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == UA;
}

void buildDISCFrame(frame_t *frame, bool transmitterToReceiver)
{
    frame->size = 5;
    frame->bytes[0] = FLAG;
    if (transmitterToReceiver)
        frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    else
        frame->bytes[1] = RECEIVER_TO_TRANSMITTER;
    frame->bytes[2] = DISC;
    frame->bytes[3] = 1;    // BCC
    frame->bytes[4] = FLAG;
}

bool isDISCFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == DISC;
}

void prepareToReceive(frame_t *frame, size_t size)
{
    frame->size = size;
}

void printFrame(frame_t *frame) {
    printf("\nStarting printFrame...\n\tSize: %ld\n", frame->size);
    for (int i = 0; i < frame->size; i++)
    {
        printf("\t0x%x ", frame->bytes[i]);
    }
    printf("\nprintString ended\n");
}

