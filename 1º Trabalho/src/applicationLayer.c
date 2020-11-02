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

application app;

void appRun() {
    if ((app.fd = llopen(app.port, app.status)) < 0) {
        printf("error in llopen\n"); 
        clearSerialPort(app.port);
        exit(1);
    }

    switch (app.status) {
        case TRANSMITTER:;
            llwrite(app.fd, "ola eu sou o joao", 17);
        break;
        case RECEIVER:;
            char *received;
            llread(app.fd, &received);
            printf("Data Received: %s\n", received);
        break;
    }

    int llcloseReturn = llclose(app.fd);
    if (llcloseReturn < 0) {
        printf("error in llclose\n"); 
        clearSerialPort(app.port);
        exit(1);
    }
}

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

    if(llwrite(app.fd, packet->bytes, packet->size) < 0){
        perror("Error transmitting start control packet in applicationLayer.c ...");
        return -1;
    }

    while((size = read(fd,buffer,MAX_DATA_PACKET_DATA_LENGTH))!=EOF){
        packet = createDataPacket(buffer, (number % 256), size);
        if(llwrite(app.fd, packet->bytes, packet->size) < 0){
            perror("Error transmitting data packet in applicationLayer.c ...");
            return -1;
        }
        number++;
    }

    packet = createControlPacket(END, fileSize, filename);

    if(llwrite(app.fd, packet->bytes, packet->size) < 0){
        perror("Error transmitting end control packet in applicationLayer.c ...");
        return -1;
    }

    fclose(fd);
    return 0;
}


packet_t * createControlPacket(u_int8_t type, int size, char * filename){
    packet_t * packet;
    packet->bytes[0] = type;
    

    packet->bytes[1] = FILESIZE;
    packet->bytes[2] = 4;
    u_int8_t byte = (size & BYTE_MASK << 24); //LSB
    packet->bytes[3] = byte;
    byte = size & (BYTE_MASK << 16);
    packet->bytes[4] = byte;
    byte = size & (BYTE_MASK << 8);
    packet->bytes[5] = byte;
    byte = size & (BYTE_MASK); //MSB
    packet->bytes[6] = byte;

    packet->bytes[7] = FILENAME;
    packet->bytes[8] = strlen(filename);          // Qual a razao de ter +1 ???? (PERGUNTA DO JOAO PINTO)
    for(int i = 0; i < packet->bytes[8] ; i++){
        packet->bytes[9 + i] = filename[i];
    }
    
    return packet;
}

int parseControlPacket(char* controlPacket, char* filename){
    int size = controlPacket[4];
    u_int8_t stringSize = controlPacket[5];
    for(int i = 0; i < stringSize; i++){
        filename[i] = controlPacket[5 + 1 + i];
    }

    return size;
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

char* parseDataPacket(char * dataArray){
    char * bytes;
    memcpy( bytes, &dataArray[3], (dataArray[3]*256 + dataArray[2]) * sizeof(*dataArray));
    return bytes;
}

int receiveFile(){
    FILE *fd;
    int error = 0;

    char* receive, filename, bytes;
    int fileSize;

    if(llread(app.fd, &receive) < 0){
        perror("Error receiving start control packet in applicationLayer.c ...");
        return -1;
    }

    fileSize = parseControlPacket(receive, filename);

    fd = fopen(filename, "w");

    for(int i = 0 ; i < fileSize ; i++){
        if(llread(app.fd, &receive) < 0){
            perror("Error receiving data packet in applicationLayer.c ...");
            return -1;
        }
        bytes = parseDataPacket(receive);
        write(fd, bytes, strlen(bytes));
    }

    if(llread(app.fd, &receive) < 0){
        perror("Error receiving end control packet in applicationLayer.c ...");
        return -1;
    }

}