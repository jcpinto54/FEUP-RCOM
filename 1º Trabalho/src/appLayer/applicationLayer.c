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
#include "../dataLayer/dataLayer.h"

extern application app;

void appRun() {
    if ((app.fd = llopen(app.port, app.status)) < 0) {
        printf("error in llopen\n"); 
        clearSerialPort(app.port);
        exit(1);
    }

    switch (app.status) {
        case TRANSMITTER:;
            sendFile(app.filename);
        break;
        case RECEIVER:;
            receiveFile();
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

    if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
        perror("Error transmitting start control packet in applicationLayer.c ...");
        return -1;
    }

    while((size = fread(buffer, sizeof(u_int8_t), MAX_DATA_PACKET_DATA_LENGTH, fd))!=EOF){
        packet = createDataPacket(buffer, (number % 256), size);
        if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
            perror("Error transmitting data packet in applicationLayer.c ...");
            return -1;
        }
        number++;
    }

    packet = createControlPacket(END, fileSize, filename);

    if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
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
    u_int8_t byte = (size & ((u_int8_t)BYTE_MASK << 24)) >> 24; //MSB
    packet->bytes[3] = byte;
    byte = (size & ((u_int8_t)BYTE_MASK << 16)) >> 16;
    packet->bytes[4] = byte;
    byte = (size & ((u_int8_t)BYTE_MASK << 8)) >> 8;
    packet->bytes[5] = byte;
    byte = size & ((u_int8_t)BYTE_MASK); //LSB
    packet->bytes[6] = byte;

    packet->bytes[7] = FILENAME;
    packet->bytes[8] = strlen(filename);         
    for(int i = 0; i < packet->bytes[8] ; i++){
        packet->bytes[9 + i] = filename[i];
    }
    
    return packet;
}

int parseControlPacket(char* controlPacket, int* fileSize, char* filename){
    int controlStatus = controlPacket[0];
    if(controlStatus != START && controlStatus != END){
        perror("Unknown control packet status: not START nor END!");
        return -1;
    }


    *fileSize = controlPacket[3] << 24 | controlPacket[4] << 16 | controlPacket[5] << 8 | controlPacket[6];
    u_int8_t stringSize = controlPacket[8];
    for(int i = 0; i < stringSize; i++){
        filename[i] = controlPacket[8 + 1 + i];
    }

    return controlStatus;
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

    return packet;
}

void * parseDataPacket(char * dataArray, char * bytes){
    return memcpy( bytes, &dataArray[3], (dataArray[3]*256 + dataArray[2]) * sizeof(*dataArray));
}

int receiveFile(){
    FILE *fd;

    char* receive, *filename, *bytes;
    int fileSize, controlStatus;

    if(llread(app.fd, &receive) < 0){
        perror("Error receiving start control packet in applicationLayer.c ...");
        return -1;
    }

    controlStatus = parseControlPacket(receive, &fileSize, filename);

    if(controlStatus != START){
        perror("Error receiving start control packet in applicationLayer.c ...");
        return -1;
    }

    fd = fopen(filename, "w");

    for(int i = 0 ; i < fileSize ; i++){
        if(llread(app.fd, &receive) < 0){
            perror("Error receiving data packet in applicationLayer.c ...");
            return -1;
        }
        parseDataPacket(receive, bytes);
        if(write(fd, bytes, strlen(bytes)) != strlen(bytes)){
            perror("Error writing to file ...");
            return -1;
        }
    }

    if(llread(app.fd, &receive) < 0){
        perror("Error receiving end control packet in applicationLayer.c ...");
        return -1;
    }

    controlStatus = parseControlPacket(receive, &fileSize, filename);

    if(controlStatus != END){
        perror("Error receiving end control packet in applicationLayer.c ...");
        return -1;
    }

    fclose(fd);

    return 0;

}