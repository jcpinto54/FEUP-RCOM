#pragma once
#include "macros.h"

typedef struct {
    int fd; // serial port
    int status;
    char port[11];
} application;


typedef struct {
    u_int8_t bytes[MAX_FRAME_SIZE];
    size_t size;
} packet_t;

void appRun();

int sendFile(char * filename);
packet_t * createDataPacket(char * string, int number, size_t size);
packet_t * createControlPacket(u_int8_t type, int size, char * filename);
char* parseDataPacket(packet_t * dataPacket);
int receiveFile(char* filename);
char* parseDataPacket(char * dataPacket);
int parseControlPacket(char* controlPacket, int* fileSize, char* filename);
int receiveFile();

