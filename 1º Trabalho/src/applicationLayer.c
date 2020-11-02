#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "applicationLayer.h"
#include "dataLayer.h"

extern application application;


int sendFile(char * filename){
    packet_t *packet;
    FILE *fd;
    char buffer[MAX_DATA_PACKET_DATA_LENGTH];
    int size = 0, number = 0;
    fd = fopen(filename, "r");

    fseek(fd, 0L, SEEK_END);
    int fileSize = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    packet = createControlPacket(START, fileSize, filename);

    if(llwrite(application.fd, packet->bytes, packet->size) < 0){
        perror("Error transmitting start control packet in applicationLayer.c ...");
        return -1;
    }

    while((size = read(fd,buffer,MAX_DATA_PACKET_DATA_LENGTH))!=EOF){
        packet = createDataPacket(buffer, (number % 256), size);
        if(llwrite(application.fd, packet->bytes, packet->size) < 0){
            perror("Error transmitting data packet in applicationLayer.c ...");
            return -1;
        }
        number++;
    }

    packet = createControlPacket(END, fileSize, filename);

    if(llwrite(application.fd, packet->bytes, packet->size) < 0){
        perror("Error transmitting end control packet in applicationLayer.c ...");
        return -1;
    }

    fclose(fd);
    return 0;
}


packet_t * createControlPacket(u_int8_t type, int size, char * filename){
    packet_t * packet;
    packet->bytes[0] = type;
    int i = 0;

    packet->bytes[1] = FILESIZE;
    u_int8_t byte = (size & BYTE_MASK); //LSB
    packet->bytes[2] = byte;
    byte = size & (BYTE_MASK << 8);
    packet->bytes[3] = byte;
    byte = size & (BYTE_MASK << 8);
    packet->bytes[4] = byte;
    byte = size & (BYTE_MASK << 8); //MSB
    packet->bytes[5] = byte;
    packet->bytes[6] = size;

    packet->bytes[7] = FILENAME;
    packet->bytes[8] = strlen(filename)+1;
    for(; i < packet->bytes[8] ; i++){
        packet->bytes[8 + i] = filename[i];
    }
    
    return packet;
}

int parseControlPacket(packet_t* controlPacket){

}

packet_t * createDataPacket(char * string, int number, size_t size){
    packet_t *packet;
    packet->size = size + 4;
    packet->bytes[0] = DATA;
    packet->bytes[1] = number;
    packet->bytes[2] = (int) (size / 256);
    packet->bytes[3] = size % 256;
    for(int i = 0; i < size ; i++){
        packet->bytes[4 + i] = string[i];
    }
}

char* parseDataPacket(char* dataPacket){

}

int receiveFile(char* filename){
    FILE *fd;

    fd = fopen(filename, "w");

    char* receive;

    if(llread(&receive) < 0){
        perror("Error receiving start control packet in applicationLayer.c ...");
        return -1;
    }

    parseControlPacket(receive);

    while()


}