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
#include "../macros.h"
#include "../utils/utils.h"

int idFrameSent = 0;
int lastFrameReceivedId = -1;
bool justRead;     // used for timeout
extern int maxFrameSize;
extern int maxFrameDataLength;

void stuffFrame(frame_t * frame)
{
    int stuffingCounter = 0;
    u_int8_t *frameRealData = (u_int8_t *)malloc(maxFrameDataLength);
    for (int i = 0; i < 6; i++) frameRealData[i] = frame->bytes[i];
    for (int i = 6; i < frame->size - 3 + stuffingCounter; i++) {
        if (frame->bytes[i - stuffingCounter] == FLAG) {
            frameRealData[i] = ESC;
            frameRealData[++i] = FLAG_STUFFING;
            stuffingCounter++;
            continue;
        }
        if (frame->bytes[i - stuffingCounter] == ESC) {
            frameRealData[i] = ESC;
            frameRealData[++i] = ESC_STUFFING;
            stuffingCounter++;
            continue;
        }
        frameRealData[i] = frame->bytes[i - stuffingCounter];
    }
    for (int i = frame->size - 3; i < frame->size; i++) frameRealData[i + stuffingCounter] = frame->bytes[i];


    frameRealData[4] += stuffingCounter / 256;
    frameRealData[5] += stuffingCounter % 256;

    frame->size += stuffingCounter;
    frameRealData[frame->size - 2] = bccCalculator(frameRealData, 4, frameRealData[4] * 256 + frameRealData[5] + 2);

    memcpy(frame->bytes, frameRealData, frame->size);
}


void destuffFrame(frame_t *frame) {
    bool destuffing = false;
    int destuffingCounter = 0;
    u_int8_t *frameRealData = (u_int8_t *)malloc(maxFrameDataLength);
    for (int i = 0; i < 6; i++) frameRealData[i] = frame->bytes[i];
    for (int i = 6; i < frame->size - 3; i++) {
        if (frame->bytes[i] == ESC) {
            destuffingCounter++;
            destuffing = true;
            continue;
        }
        if (destuffing && frame->bytes[i] == FLAG_STUFFING) {
            frameRealData[i - destuffingCounter] = FLAG;
            destuffing = false;
            continue;
        }
        else if (destuffing && frame->bytes[i] == ESC_STUFFING) {
            frameRealData[i - destuffingCounter] = ESC;
            destuffing = false;
            continue;
        }
        frameRealData[i - destuffingCounter] = frame->bytes[i];
    }
    for (int i = frame->size-3; i < frame->size; i++) frameRealData[i - destuffingCounter] = frame->bytes[i];
    
    frameRealData[4] -= destuffingCounter / 256;
    frameRealData[5] -= destuffingCounter % 256;

    frame->size -= destuffingCounter;
    frameRealData[frame->size - 2] = bccCalculator(frameRealData, 4, frameRealData[4] * 256 + frameRealData[5] + 2);

    memcpy(frame->bytes, frameRealData, frame->size);
}


// pode ser necessário ter os dados em mais que uma frame
frame_t prepareI(char* data, int length) //Testar
{
    u_int8_t frameDataSize[2];
    frame_t info;
    info.bytes = (u_int8_t *)malloc(maxFrameSize);

        //debug
    info.bytes[0] = FLAG; //F
    info.bytes[1] = TRANSMITTER_TO_RECEIVER; //A
    info.bytes[2] = idFrameSent << 6 | I;
    info.infoId = idFrameSent;
    info.bytes[3] = bccCalculator(info.bytes, 1, 2); //BCC1, calculado com A e C
    

    prepareFrameDataSize(length, frameDataSize);

    info.bytes[4] = frameDataSize[0];
    info.bytes[5] = frameDataSize[1];

    for (int j = 0; j < length; j++) {
        info.bytes[6 + j] = data[j];
    }

    int bcc2_byte_ix = 4 + 2 + (info.bytes[4] << 8)  + info.bytes[5];

    info.bytes[bcc2_byte_ix] = bccCalculator(info.bytes, 4, (info.bytes[4] << 8) + info.bytes[5] + 2);  
    info.bytes[bcc2_byte_ix + 1] = FLAG;
    printf("I: %d   -   info: %d", bcc2_byte_ix, info.bytes[bcc2_byte_ix]);
    info.size = 4 + 2 + info.bytes[4] * 256 + info.bytes[5] + 2;


    stuffFrame(&info);

    idFrameSent = (idFrameSent + 1) % 2;

    return info;
    
}

void readTimeoutHandler(int signo) {
    if (!justRead) {
        printf("Timeout occured while reading a frame. Exiting Program...\n");
        exit(1);
    }
}


// Returns -4 if there was a timeout
// Returns -3 if there is an error with reading from the serial port
// Returns -2 if there is an error with bcc2 
// Returns -1 if there is an error with data size value
// Returns 0 if received ok
// Returns 1 if received a repeated frame
int receiveIMessage(frame_t *frame, int fd, int timeout){
    u_int8_t c;
    receive_state_t state = INIT;
    int dataCounter = -2, returnValue = 0;
    printf("1\n");
    do {
        printf("1\n");
        alarm(3); 
        justRead = false;
        int bytesRead = read(fd, &c, 1);
        justRead = true;
        
        printf("byte: %x   -   state: %d  -  bytesRead: %d\n", c, state, bytesRead);
        if (bytesRead < 0) {
            perror("read error");
            return -3;
        }
        printf("1\n");
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
                else if (c == FLAG) {
                    state = RCV_FLAG;
                }
                else {
                    state = INIT;
                }
                break;
            case RCV_A:
                if ((c & I_MASK) == I) {
                    state = RCV_C;
                    frame->bytes[2] = c;
                    frame->infoId = c >> 6; 
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
                    state = INIT;
                }
                break;
            case RCV_BCC1:     
                frame->bytes[4 + 2 + dataCounter] = c;
                dataCounter++;
                if (dataCounter == (frame->bytes[4] * 256 + frame->bytes[5])) state = RCV_DATA;
                break;
            case RCV_DATA:
                // printf("REPEATED byte: %x   -   state: %d   -   bcc: %x\n", c, state, bccCalculator(frame->bytes, 4, dataCounter + 2));
                if (bccVerifier(frame->bytes, 4, dataCounter + 2, c)) {
                    state = RCV_BCC2;
                    frame->bytes[4 + 2 + dataCounter] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                else {
                    printf("DATA - BCC2 not correct\n");
                    returnValue = -2;
                }
                break;
            case RCV_BCC2:
                // printf("byte: %x   -   state: %d\n", c, state);
                if (c == FLAG) {
                    state = COMPLETE;
                    frame->bytes[4 + 2 + dataCounter + 1] = c;
                }
                else
                    state = INIT;
                break;
            case COMPLETE: break;
        }
        printf("1\n");
        // sleep(1);
    } while (state != COMPLETE && returnValue == 0);

    printf("1\n");
    if (lastFrameReceivedId != -1 && lastFrameReceivedId == frame->infoId && returnValue == 0) {
        printf("DATA - Read a duplicate frame\n");
        returnValue = 1;
    }
    else if (returnValue == 0) {
        frame->size = 4 + 2 + dataCounter + 1 + 1;
        destuffFrame(frame);
        returnValue = 0;
    }
    printf("1\n");

    return returnValue;
}

// Returns 0 if received ok
// Returns 1 if received RR ok
// Returns 2 if received REJ ok
// Returns -1 if there was a timeout
// Returns -2 if there was a read error
int receiveNotIMessage(frame_t *frame, int fd, int responseId, int timeout)
{
    u_int8_t c;
    receive_state_t state = INIT;
    do {
        alarm(3); 
        justRead = false;
        int bytesRead = read(fd, &c, 1);
        justRead = true;
        printf("byte: %x   -   state: %d\n", c, state);
        if (bytesRead < 0) {
            perror("read error");
            return -2;
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
                else if (c == FLAG) {
                    state = RCV_FLAG;
                }
                else {
                    state = INIT;
                    return -3;
                }
                break;
            case RCV_A:
                if ((c == SET) || (c == UA) || (c == DISC) || (c == (RR | (responseId << 7))) || (c == (REJ | (responseId << 7)))) {
                    state = RCV_C;
                    frame->bytes[2] = c;
                }
                else if (c == FLAG) {
                    state = RCV_FLAG;
                }
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
                    printf("BCC1 not correct\n");
                    state = INIT;
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
                printf("DATA - Unknown state");
                state = INIT;
                break;
        }
        // sleep(1);
    } while (state != COMPLETE);
    frame->size = 5;
    int returnValue = 0;
    if (frame->bytes[2] == (RR | (responseId << 7))) returnValue = 1;
    if (frame->bytes[2] == (REJ | (responseId << 7))) returnValue = 2;
    return returnValue;
}

// Returns -1 if timeout, 0 if ok 
int sendNotIFrame(frame_t *frame, int fd) {
    int writeReturn = write(fd, frame->bytes, frame->size);   
    printf("DATA - %d bytes sent\n", writeReturn);

    if (writeReturn == -1) return -1; 
    return 0;
}

// Returns -1 if max write attempts were reached
// Returns 0 if ok
int sendIFrame(frame_t *frame, int fd) {
    int attempts = 0, sentBytes = 0;
    frame_t responseFrame;
    responseFrame.bytes = (u_int8_t *)malloc(maxFrameSize);
    while (1) {
        if(attempts >= MAX_WRITE_ATTEMPTS) 
        {
            printf("DATA - Too many write attempts\n");
            return -1;
        }
        if ((sentBytes = write(fd, frame->bytes, frame->size)) == -1) return -1;                  
        printf("DATA - %d bytes sent\n", sentBytes);

        //debug
        /*
        printf("sentFrame size: %lu\n", frame->size);
        printf("sentFrame flag: %x\n", frame->bytes[0] & 0xff);
        printf("sentFrame a: %x\n", frame->bytes[1]& 0xff);
        printf("sentFrame c: %x\n", frame->bytes[2]& 0xff);
        printf("sentFrame bcc: %x\n", frame->bytes[3]& 0xff);
        printf("sentFrame flag: %x\n\n\n", frame->bytes[4]& 0xff);
        */
        int receiveReturn = receiveNotIMessage(&responseFrame, fd, (frame->infoId + 1) % 2, 2);
        // debug
        /*
        printf("responseFrame size: %lu\n", responseFrame.size);
        printf("responseFrame flag: %x\n", responseFrame.bytes[0] & 0xff);
        printf("responseFrame a: %x\n", responseFrame.bytes[1]& 0xff);
        printf("responseFrame c: %x\n", responseFrame.bytes[2]& 0xff);
        printf("responseFrame bcc: %x\n", responseFrame.bytes[3]& 0xff);
        printf("responseFrame flag: %x\n\n\n", responseFrame.bytes[4]& 0xff);
        */
        if (receiveReturn == -1) {
            printf("DATA - Timeout reading response, trying again...\n");
        }
        else if (receiveReturn == -2) {
            printf("DATA - There was a reading error while reading response, trying again...\n");
        }
        else if (receiveReturn == 0) {
            printf("DATA - Received wrong response, trying again...\n");
        }
        else if (receiveReturn == 1) {
            printf("DATA - Received an OK message from the receiver.\n");
            break;
        }
        else if (receiveReturn == 2) {
            printf("DATA - Received not OK message from the receiver, trying again...\n");
        }
        else {
            printf("DATA - Unexpected response frame, trying again...\n");
        }
        attempts++;
    }
    return 0;
}

void prepareResponse(frame_t *frame, bool valid, int id) {
    frame->size = 5;
    frame->bytes[0] = FLAG;
    frame->bytes[1] = TRANSMITTER_TO_RECEIVER;
    if (valid) 
        frame->bytes[2] = RR | (id << 7);
    else 
        frame->bytes[2] = REJ | (id << 7);
    frame->bytes[3] = bccCalculator(frame->bytes, 1, 2);
    frame->bytes[4] = FLAG;
}

// ---

u_int8_t bccCalculator(u_int8_t bytes[], int start, int length)
{
    int bcc = 0x00;
    for (int i = start; i < start + length; i++)
    {
        bcc ^= bytes[i];
    }
    return bcc;
}

// Return true if bcc verifies else otherwise 
bool bccVerifier(u_int8_t bytes[], int start, int length, u_int8_t parity)
{
    return (bccCalculator(bytes, start, length) == parity);
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
    frame->bytes[3] = bccCalculator(frame->bytes, 1, 2);    // BCC
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
    frame->bytes[3] = bccCalculator(frame->bytes, 1, 2);    // BCC
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
    frame->bytes[3] = bccCalculator(frame->bytes, 1, 2);    // BCC
    frame->bytes[4] = FLAG;
}

bool isDISCFrame(frame_t *frame) {
    if (frame->size != 5) return false;
    return frame->bytes[2] == DISC;
}

void prepareToReceive(frame_t *frame, int size)
{
    frame->size = size;
}

void prepareFrameDataSize(int frameSize, u_int8_t *sizeBytes) {
    sizeBytes[0] = (u_int8_t) (frameSize >> 8);
    sizeBytes[1] = (u_int8_t) frameSize;
}

void printFrame(frame_t *frame) {
    printf("\nStarting printFrame...\n\tSize: %d\n", frame->size);
    for (int i = 0; i < frame->size; i++)
    {
        printf("\tByte %d: %x \n", i, frame->bytes[i]);
    }
    printf("printFrame ended\n\n");
}

