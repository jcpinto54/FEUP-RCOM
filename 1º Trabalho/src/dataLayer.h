#pragma once
#include "utils.h"

typedef struct {
    int fd; // serial port
    int status; // TRANSMITTER | RECEIVER
} applicationLayer;


typedef enum {
    INIT,
    RCV_FLAG,
    RCV_A,
    RCV_C,
    RCV_BCC,
    COMPLETE
} receive_state_t;

typedef struct {
    u_int8_t *bytes;
    int size;
} frame_t;


// ---


applicationLayer app;

u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length);
bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity);

int prepareToReceive(frame_t *frame, size_t size);
int buildSETFrame(frame_t *frame, bool transmitterToReceiver);
int buildUAFrame(frame_t * frame, bool transmitterToReceiver);
int buildDISCFrame(frame_t * frame, bool transmitterToReceiver);
void destroyFrame(frame_t *frame);
int receiveNotIMessage(frame_t *frame);
int sendMessage(frame_t frame);

int llopen(char *port, int appStatus);
int llclose(int fd);

void printString(char * str);
