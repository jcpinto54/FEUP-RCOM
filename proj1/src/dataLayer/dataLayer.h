#pragma once
#include "../macros.h"

typedef struct {
    u_int8_t **bytes;   // we had memory problems, and got it working this way: we used it an array of bytes
    int size;       
    int infoId;         // frame id, if needed
} frame_t;      


int llopen(char *port, int appStatus);
int llclose(int fd);
int llread(int fd, char *buffer);
int llwrite(int fd, char * buffer, int length);

int clearSerialPort(char *port);

int convertBaudrate(int baudArg);
