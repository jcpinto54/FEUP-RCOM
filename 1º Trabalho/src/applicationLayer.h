#pragma once
#include "macros.h"

typedef struct {
    u_int8_t bytes[MAX_FRAME_SIZE];
    size_t size;
} packet_t;


int parseFileData(char * filename);
packet_t * createDataPacket(char * string, size_t size);
int parseDataPacket(packet_t * dataPacket);