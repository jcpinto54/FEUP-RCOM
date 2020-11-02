#pragma once
#include "macros.h"

typedef struct {
    u_int8_t bytes[MAX_FRAME_SIZE];
    size_t size;
    int infoId;
} frame_t;



int llopen(char *port, int appStatus);
int llclose(int fd);
int llread(int fd, char ** buffer);
int llwrite(int fd, char * buffer, int length);

int clearSerialPort(char *port);
