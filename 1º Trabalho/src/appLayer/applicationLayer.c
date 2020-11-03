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
#include "../utils/utils.h"

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
    char buffer[MAX_PACKET_DATA_LENGTH];
    int size = 0, number = 0;
    fd = fopen(filename, "r");

    fseek(fd, 0L, SEEK_END);
    int fileSize = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    packet = createControlPacket(START, fileSize, filename);

    if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
        printf("Error transmitting start control packet in applicationLayer.c ...\n");
        return -1;
    }

    while((size = fread(buffer, sizeof(u_int8_t), MAX_PACKET_DATA_LENGTH, fd))!=EOF){
        packet = createDataPacket(buffer, (number % 256), size);
        if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
            printf("Error transmitting data packet in applicationLayer.c ...\n");
            return -1;
        }
        number++;
    }

    packet = createControlPacket(END, fileSize, filename);

    if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
        printf("Error transmitting end control packet in applicationLayer.c ...\n");
        return -1;
    }

    fclose(fd);
    return 0;
}


packet_t * createControlPacket(u_int8_t type, int size, char * filename){
    packet_t * packet = (packet_t *)malloc(sizeof(packet_t));
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
    packet->size = 8 + packet->bytes[8];
    return packet;
}

int parseControlPacket(char* controlPacket, int* fileSize, char* filename){
    int controlStatus = controlPacket[0];
    printf("controlStatus: %x\n", controlStatus);
    if(controlStatus != START && controlStatus != END){
        printf("Unknown control packet status: not START nor END!\n");
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
    packet_t *packet = (packet_t *)malloc(sizeof(packet_t));
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
        printf("Error receiving start control packet in applicationLayer.c ...\n");
        return -1;
    }

    filename = (char *)malloc(MAX_FILENAME_LENGTH);
    controlStatus = parseControlPacket(receive, &fileSize, filename);  // fileSize is returning negative numbers

    printf("fileSize: %d   -  conta de merda: %d  -  fileName: %s\n", fileSize, ceiling(1.0/(MAX_PACKET_DATA_LENGTH/(float)fileSize)), filename);
    // for (int i = 0; )

    if(controlStatus != START){
        printf("Error receiving start control packet in applicationLayer.c ...\n");
        return -1;
    }

    strcpy(filename, "output");
    fd = fopen(filename, "w");

    for(int i = 0 ; i < (fileSize / MAX_PACKET_DATA_LENGTH) + 1 ; i++){
        if(llread(app.fd, &receive) < 0){
            printf("Error receiving data packet in applicationLayer.c ...\n");
            return -1;
        }
        bytes = (char *)malloc((receive[3]*256 + receive[2]));
        parseDataPacket(receive, bytes);
        
        if(fwrite(bytes, sizeof(u_int8_t), strlen(bytes), fd) != strlen(bytes)){
            perror("Error writing to file ...");
            return -1;
        }
    }

    if(llread(app.fd, &receive) < 0){
        printf("Error receiving end control packet in applicationLayer.c ...\n");
        return -1;
    }

    controlStatus = parseControlPacket(receive, &fileSize, filename);

    if(controlStatus != END){
        printf("Error receiving end control packet in applicationLayer.c ...\n");
        return -1;
    }

    fclose(fd);
    free(receive);
    free(filename);
    free(bytes);
    return 0;

}

void printPacket(packet_t *packet) {
    printf("\nStarting printPacket...\n\tSize: %ld\n", packet->size);
    for (int i = 0; i < packet->size; i++)
    {
        printf("\tByte %d: %x \n", i, packet->bytes[i]);
    }
    printf("printPacket ended\n\n");
}
