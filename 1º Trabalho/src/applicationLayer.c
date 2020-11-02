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

extern applicationLayer application;


int sendFile(char * filename){
  
    packet_t *packet;
    FILE *fd;
    char buffer[MAX_DATA_PACKET_DATA_LENGTH];
    int size = 0;
    fd = fopen(filename, "r");

    fseek(fd, 0L, SEEK_END);
    int fileSize = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    packet = createControlPacket(START, fileSize, filename);

    if(llwrite(application.fd, packet->bytes, packet->size) == -1){
        perror("Error transmitting start control packet in applicationLayer.c ...");
        return -1;
    }

    while((size = read(fd,buffer,MAX_DATA_PACKET_DATA_LENGTH))!=EOF){
        packet = createDataPacket(buffer,size);
        if(llwrite(application.fd, packet->bytes, packet->size) == -1){
            perror("Error transmitting data packet in applicationLayer.c ...");
            return -1;
        }
    }

    packet = createControlPacket(END, fileSize, filename);

    if(llwrite(application.fd, packet->bytes, packet->size) == -1){
        perror("Error transmitting end control packet in applicationLayer.c ...");
        return -1;
    }

    fclose(fd);

    return 0;

    //TODO dividir a file data em pacotes de tamanho igual ao MAX_DATA_PACKET_LENGTH
    //criar um packet para cada fragmento de dados chamando createDataPacket
}


packet_t * createControlPacket(u_int8_t type, int size, char * filename){
    packet_t * packet;
    packet->bytes[0] = type;
    int i = 0;
    packet->bytes[1] = FILENAME;
    packet->bytes[2] = strlen(filename)+1;
    for(; i < packet->bytes[2] ; i++){
        packet->bytes[3 + i] = filename[i];
    }
    i = 3 + i;


    u_int8_t byte = (size & BYTE_MASK); //LSB
    packet->bytes[i++] = byte;

    byte = size & (BYTE_MASK << 8);
    packet->bytes[i++] = byte;

    byte = size & (BYTE_MASK << 8);
    packet->bytes[i++] = byte;

    byte = size & (BYTE_MASK << 8); //MSB
    packet->bytes[i] = byte;


    return packet;
    /*int numOfDigits = log10(size) + 1; 
    char* arr = calloc(numOfDigits, sizeof(char));
    for(int x = 0 ; x< numOfDigits; x++, size/=10) 
    { 
	    arr[x] = size % 10; 
    }

    packet->bytes[++i] = FILESIZE;
    packet->bytes[++i] = strlen(arr) + 1;
    int bytelocation = i;
    for(; i < packet->bytes[bytelocation] ; i++){
        packet->bytes[bytelocation + i] = arr[i];
    }*/

}

packet_t * createDataPacket(char * string, size_t size){
    packet_t *packet;
    packet->size = size + 4;
    packet->bytes[0] = DATA;
    packet->bytes[1] = 1;
    packet->bytes[2] = (int) (size / 256);
    packet->bytes[3] = size % 256;
    for(int i = 0; i < size ; i++){
        packet->bytes[4 + i] = string[i];
    }
}

int parseDataPacket(packet_t * dataPacket){

}

int receiveFile(){

}