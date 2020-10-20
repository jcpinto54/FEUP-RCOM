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
#include "2ndAssignment.h"


void receiveUA();
void sendSET();
int writeFrame(frame_t frame);
int readFrame(frame_t *frame);
void llopen(int porta, int appStatus);

#endif