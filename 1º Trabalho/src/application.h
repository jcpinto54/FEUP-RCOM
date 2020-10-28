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



u_int8_t bccCalculator(u_int8_t bytes[], int start, size_t length);
bool bccVerifier(u_int8_t bytes[], int start, size_t length, u_int8_t parity);

int prepareToReceive(frame_t *frame, size_t size);
void buildSETFrame(frame_t *frame, bool transmitterToReceiver);
void buildUAFrame(frame_t * frame, bool transmitterToReceiver);
void auxStuffing(frame_t * frame, int * stuffingCounter, char byte, int i);
int prepareI(char* data, int size, frame_t* info);
void destroyFrame(frame_t *frame);
void receiveNotIMessage(frame_t *frame);
void sendMessage(frame_t frame);

void llopen(char *port, int appStatus);
int llwrite(int fd, char * buffer, int length);


void printString(char * str);
