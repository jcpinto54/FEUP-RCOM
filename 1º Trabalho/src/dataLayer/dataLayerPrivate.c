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

void stuffFrame(frame_t * frame)
{
    int stuffingCounter = 0;
    u_int8_t frameRealData[MAX_FRAME_SIZE];
    for (int i = 0; i < 5; i++) frameRealData[i] = frame->bytes[i];
    for (int i = 5; i < frame->size - 3 + stuffingCounter; i++) {
        if (frame->bytes[i - stuffingCounter] == FLAG) {
            frameRealData[i] = ESC;
            frameRealData[++i] = FLAG_STUFFING;
            frameRealData[4]++;
            stuffingCounter++;
            continue;
        }
        if (frame->bytes[i - stuffingCounter] == ESC) {
            frameRealData[i] = ESC;
            frameRealData[++i] = ESC_STUFFING;
            frameRealData[4]++;
            stuffingCounter++;
            continue;
        }
        frameRealData[i] = frame->bytes[i - stuffingCounter];
    }
    for (int i = frame->size - 3; i < frame->size; i++) frameRealData[i + stuffingCounter] = frame->bytes[i];
    frame->size += stuffingCounter;
    frameRealData[frame->size - 2] = bccCalculator(frameRealData, 4, frameRealData[4] + 2);

    memcpy(frame->bytes, frameRealData, frame->size);
}


void destuffFrame(frame_t *frame) {
    bool destuffing = false;
    int destuffingCounter = 0;
    u_int8_t frameRealData[MAX_FRAME_SIZE];
    for (int i = 0; i < 5; i++) frameRealData[i] = frame->bytes[i];
    for (int i = 5; i < frame->size - 3; i++) {
        if (frame->bytes[i] == ESC) {
            destuffingCounter++;
            destuffing = true;
            frameRealData[4]--;
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
    frame->size -= destuffingCounter;
    frameRealData[frame->size - 2] = bccCalculator(frameRealData, 4, frameRealData[4] + 2);

    memcpy(frame->bytes, frameRealData, frame->size);
}


// pode ser necessário ter os dados em mais que uma frame
int prepareI(char* data, int length, frame_t *** infoNew) //Testar
{
    int framesNeeded = ceiling(1.0/(MAX_FRAME_DATA_LENGTH/(float)length));      // Division has got to be like this   

    u_int8_t frameDataSize;
    frame_t **info = malloc(sizeof(frame_t *) * framesNeeded);
    for (int i = 0; i < framesNeeded; i++) {
        info[i] = malloc(sizeof(frame_t));

        //debug
        info[i]->bytes[0] = FLAG; //F
        printf("Flag: %x\n", info[i]->bytes[0]& 0xff);
        info[i]->bytes[1] = TRANSMITTER_TO_RECEIVER; //A
        printf("A: %x\n", info[i]->bytes[1]& 0xff);
        info[i]->bytes[2] = idFrameSent << 6 | I;
        printf("C: %x\n", info[i]->bytes[2]& 0xff);
        info[i]->infoId = idFrameSent;
        info[i]->bytes[3] = bccCalculator(info[i]->bytes, 1, 2); //BCC1, calculado com A e C
        printf("BCC1: %x\n", info[i]->bytes[3]& 0xff);
        
    
        unsigned lengthInOtherFrames = 0;
        if (i < framesNeeded - 1 && framesNeeded != 1) frameDataSize = MAX_FRAME_DATA_LENGTH;
        else {
            for (int j = 0; j < i; j++) {
                lengthInOtherFrames += info[j]->bytes[4];
            }
            frameDataSize = length - lengthInOtherFrames;
        }

        info[i]->bytes[4] = frameDataSize;
        for (int j = 0; j < frameDataSize; j++) {
            info[i]->bytes[5 + j] = data[j];
        }


        int bcc2_byte_ix = 4 + 1 + frameDataSize + 1;

        if (i == framesNeeded - 1) {
            info[i]->bytes[bcc2_byte_ix - 1] = FLAG_LAST_FRAME;        
        }
        else {
            info[i]->bytes[bcc2_byte_ix - 1] = FLAG_MORE_FRAMES_TO_COME;
        }
        
        info[i]->bytes[bcc2_byte_ix] = bccCalculator(info[i]->bytes, 4, info[i]->bytes[4] + 2);  
        info[i]->bytes[bcc2_byte_ix + 1] = FLAG;
        info[i]->size = 8 + frameDataSize;

        stuffFrame(info[i]);

        data += frameDataSize;
        idFrameSent = (idFrameSent + 1) % 2;
    }
    *infoNew = info;
    return framesNeeded;
    
}


// Returns -5 if there was a timeout
// Returns -4 if there is an error with reading from the serial port
// Returns -3 if there is an error with bcc2 
// Returns -2 if there is an error with data size value
// Returns -1 if there is an error with bcc1 
// Returns 0 if there are no more frames to be read
// Returns 1 if there are more frames to be read 
// Returns 2 if received a repeated frame and there are no more frames to be read
// Returns 3 if received a repeated frame and there are more frames to be read
int receiveIMessage(frame_t *frame, int fd, int timeout){
    u_int8_t c;
    receive_state_t state = INIT;
    int dataCounter = -1, returnValue = 0;
    time_t initTime, curTime;
    initTime = time(NULL);
    do {
        int bytesRead = read(fd, &c, 1);
        if (bytesRead < 0) {
            perror("read error");
            return -4;
        }
        else if (bytesRead > 0) {
            initTime = time(NULL);
        }
        curTime = time(NULL);
        time_t seconds = curTime - initTime;
        if (seconds >= timeout) {
            return -5;
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
                else
                    state = INIT;
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
                if (dataCounter == -1 && c > 0 && c <= MAX_FRAME_DATA_LENGTH * 2) {    // *2 because of possibility of byte stuffing in every byte
                    frame->bytes[4] = c;
                    dataCounter++;
                    continue;
                }
                else if (dataCounter == -1 && (c < 0 || c > MAX_FRAME_DATA_LENGTH * 2)) {
                    printf("DATA - First item is not a valid size value    -   frameDataSize: %d\n", c);
                    returnValue = -2;
                    break;
                }
                frame->bytes[5 + dataCounter] = c;
                dataCounter++;
                if (dataCounter == frame->bytes[4]) state = RCV_DATA;
                break;
            case RCV_DATA:
                if (c == FLAG_MORE_FRAMES_TO_COME || c == FLAG_LAST_FRAME) {
                    state = RCV_LAST_FRAME_FLAG;
                    frame->bytes[4 + dataCounter + 1] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                break;            
            case RCV_LAST_FRAME_FLAG:
                if (bccVerifier(frame->bytes, 4, dataCounter + 2, c)) {
                    state = RCV_BCC2;
                    frame->bytes[4 + dataCounter + 2] = c;
                }
                else if (c == FLAG)
                    state = RCV_FLAG;
                else {
                    printf("DATA - BCC2 not correct\n");
                    returnValue = -3;
                }
                break;
            case RCV_BCC2:
                if (c == FLAG) {
                    state = COMPLETE;
                    frame->bytes[4 + dataCounter + 3] = c;
                }
                else
                    state = INIT;
                break;
            case COMPLETE: break;
        }
    } while (state != COMPLETE && returnValue == 0);
    if (lastFrameReceivedId != -1 && lastFrameReceivedId == frame->infoId && returnValue == 0) {
        printf("DATA - Read a duplicate frame\n");
        if (frame->bytes[4 + dataCounter + 1] == FLAG_LAST_FRAME) returnValue = 2;
        else if (frame->bytes[4 + dataCounter + 1] == FLAG_MORE_FRAMES_TO_COME) returnValue = 3;
    }
    else if (returnValue == 0) {
        frame->size = 5 + dataCounter + 3;
        destuffFrame(frame);
        if (frame->bytes[4 + dataCounter + 1] == FLAG_LAST_FRAME) returnValue = 0;
        else if (frame->bytes[4 + dataCounter + 1] == FLAG_MORE_FRAMES_TO_COME) returnValue = 1;
    }

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
    time_t initTime, curTime;
    initTime = time(NULL);
    do {
        int bytesRead = read(fd, &c, 1);
        if (bytesRead < 0) {
            perror("read error");
            return -2;
        }
        else if (bytesRead > 0) {
            initTime = time(NULL);
        }
        curTime = time(NULL);
        time_t seconds = curTime - initTime;
        if (seconds >= timeout) {
            return -1;
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
                    return -1;
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
    if (writeReturn == -1) return -1; 
    return 0;
}

// Returns -1 if max write attempts were reached
// Returns 0 if ok
int sendIFrame(frame_t *frame, int fd) {
    int attempts = 0, sentBytes = 0;
    frame_t responseFrame;
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

u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length)
{
    int bcc = 0x00;
    for (int i = start; i < start + length; i++)
    {
        bcc ^= bytes[i];
    }
    return bcc;
}

// Return true if bcc verifies else otherwise 
bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity)
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

void prepareToReceive(frame_t *frame, size_t size)
{
    frame->size = size;
}

void printFrame(frame_t *frame) {
    printf("\nStarting printFrame...\n\tSize: %ld\n", frame->size);
    for (int i = 0; i < frame->size; i++)
    {
        printf("\tByte %d: %x \n", i, frame->bytes[i]);
    }
    printf("printFrame ended\n\n");
}

