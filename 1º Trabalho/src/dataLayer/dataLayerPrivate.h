#pragma once
#include "../utils/utils.h"

typedef enum {
    INIT,
    RCV_FLAG,
    RCV_A,
    RCV_C,
    RCV_BCC1,
    RCV_DATA,
    RCV_BCC2,
    COMPLETE
} receive_state_t;


// ---

int receiveIMessage(frame_t *frame, int fd, int timeout);
int receiveNotIMessage(frame_t *frame, int fd, int responseId, int timeout);
int sendIFrame(frame_t *frame, int fd);
int sendNotIFrame(frame_t *frame, int fd);


u_int8_t bccCalculator(u_int8_t bytes[], int start, int length);
bool bccVerifier(u_int8_t bytes[], int start, int length, u_int8_t parity);

void buildSETFrame(frame_t *frame, bool transmitterToReceiver);
bool isSETFrame(frame_t *frame);
void buildUAFrame(frame_t * frame, bool transmitterToReceiver);
bool isUAFrame(frame_t *frame);
void buildDISCFrame(frame_t * frame, bool transmitterToReceiver);
bool isDISCFrame(frame_t *frame);
void stuffFrame(frame_t * frame);
void destuffFrame(frame_t *frame);

frame_t prepareI(char* data, int size);
void prepareResponse(frame_t *frame, bool valid, int id);

void prepareFrameDataSize(int frameSize, u_int8_t *sizeBytes);
void prepareToReceive(frame_t *frame, int size);
void printFrame(frame_t *frame);

void readTimeoutHandler(int signo);

