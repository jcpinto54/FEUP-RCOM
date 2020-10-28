#ifndef APPLICATION_H
#define APPLICATION_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "macros.h"

applicationLayer app;

u_int8_t getBit(u_int8_t byte, u_int8_t bit);
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

#endif