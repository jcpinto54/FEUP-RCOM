#pragma once
#include "../macros.h"

typedef struct {
    int fd; // serial port file descriptor
    int status; // transmitter or receiver
    char port[20];  // port name
    char filename[MAX_FILENAME_LENGTH];  // name of file to send
} application;


typedef struct {
    u_int8_t **bytes;   // we had memory problems, and got it working this way: we used it an array of bytes
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

