#pragma once
#include "../macros.h"

typedef struct {
    int fd; // serial port
    int status;
    char port[20];
    char filename[MAX_FILENAME_LENGTH];
} application;


typedef struct {
    u_int8_t bytes[MAX_FRAME_SIZE];
    int size;
} packet_t;

void appRun();

int sendFile(char * filename);
packet_t createDataPacket(u_int8_t * string, int number, int size);
packet_t createControlPacket(u_int8_t type, unsigned size, char * filename);
int parseDataPacket(u_int8_t * packetArray, u_int8_t * bytes);
int parseControlPacket(u_int8_t* controlPacket, unsigned* fileSize, char* filename);
int receiveFile();
void printPacket(packet_t *packet);

