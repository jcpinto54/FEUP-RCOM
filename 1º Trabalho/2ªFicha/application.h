#ifndef APPLICATION_H
#define APPLICATION_H

#include "macros.h"

applicationLayer app;

uint8_t getBit(uint8_t byte, uint8_t bit);
uint8_t bccCalculator(uint8_t bytes[], int start, size_t length);
bool bccVerifier(uint8_t bytes[], int start, size_t length, uint8_t parity);

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

#endif