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
extern int maxFrameDataLength;
extern int maxPacketLength;
extern int maxPacketDataLength;

void appRun() {
    if ((app.fd = llopen(app.port, app.status)) < 0) {
        printf("error in llopen: %d\n", app.fd); 
       clearSerialPort(app.port);
        exit(1);
    }
    printf("done llopen\n");
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
        printf("error in llclose: %d\n", llcloseReturn); 
       clearSerialPort(app.port);
        exit(1);
    }
}

int sendFile(char * filename){
    packet_t *packet;
    int fileFd;

    fileFd = open(filename, O_RDONLY | O_NONBLOCK);
    if (fileFd == -1) {
        perror("file not opened correctly");
        return -1;
    }

    struct stat st;
    stat(filename, &st);
    unsigned fileSize = st.st_size;

    packet = createControlPacket(START, fileSize, filename);

    if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
        printf("APP - Error transmitting start control packet in applicationLayer.c ...\n");
        return -1;
    }

    int size = 0, number = 0;
    u_int8_t buffer[maxPacketLength];
    while((size = read(fileFd, (char *)buffer, maxPacketLength)) > 0){
        packet = createDataPacket(buffer, (number % 256), size);
        if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
            printf("APP - Error transmitting data packet in applicationLayer.c ...\n");
            return -1;
        }
        number++;
    }

    packet = createControlPacket(END, fileSize, filename);

    if(llwrite(app.fd, (char *)packet->bytes, packet->size) < 0){
        printf("APP - Error transmitting end control packet in applicationLayer.c ...\n");
        return -1;
    }

    close(fileFd);
    return 0;
}


packet_t * createControlPacket(u_int8_t type, unsigned size, char * filename){
    packet_t * packet = (packet_t *)malloc(sizeof(packet_t));
    packet->bytes[0] = type;
    
    packet->bytes[1] = FILESIZE;
    packet->bytes[2] = 4;
    packet->bytes[3] = (u_int8_t)(size >> 24);
    packet->bytes[4] = (u_int8_t)(size >> 16);
    packet->bytes[5] = (u_int8_t)(size >> 8);
    packet->bytes[6] = (u_int8_t)size; //LSB

    packet->bytes[7] = FILENAME;
    packet->bytes[8] = strlen(filename) + 1;         // got to have +1
    for(int i = 0; i < packet->bytes[8] ; i++){
        packet->bytes[9 + i] = filename[i];
    }
    packet->size = 8 + packet->bytes[8];

    return packet;
}

int parseControlPacket(u_int8_t* controlPacket, unsigned* fileSize, char* filename){
    int controlStatus = controlPacket[0];
    if(controlStatus != START && controlStatus != END){
        printf("APP - Unknown control packet status: %d - not START nor END!\n" , controlStatus);
        return -1;
    }

    *fileSize = controlPacket[3] << 24 | controlPacket[4] << 16 | controlPacket[5] << 8 | controlPacket[6];
    u_int8_t stringSize = controlPacket[8];
    for(int i = 0; i < stringSize; i++){
        filename[i] = controlPacket[8 + 1 + i];
    }

    return controlStatus;
}

packet_t * createDataPacket(u_int8_t * string, int number, size_t size){
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

int parseDataPacket(u_int8_t * packetArray, u_int8_t * bytes) {
    int packetDataSize = packetArray[2]*256 + packetArray[3];
    memcpy(bytes, packetArray + 4, packetDataSize);
    return packetDataSize;
}

int receiveFile(){

    char receive[maxFrameDataLength];
    if(llread(app.fd, receive) < 0){
        printf("APP - Error receiving start control packet in applicationLayer.c ...\n");
        return -1;
    }

    unsigned fileSize;
    int controlStatus;
    char filename[MAX_FILENAME_LENGTH];
    controlStatus = parseControlPacket((u_int8_t *)receive, &fileSize, filename); 

    if(controlStatus != START){
        printf("APP - Error receiving start control packet in applicationLayer.c ...\n");
        return -1;
    }

   strcpy(filename, "output.gif");    // comment to test in the same pc 
    
    int fileFd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXG | S_IRWXU | S_IRWXO);
    if (fileFd == -1) {
        perror("file not opened correctly");
        return -1;
    }

    u_int8_t bytes[maxPacketLength];
    for(int i = 0 ; i < (fileSize / maxPacketDataLength) + 1 ; i++){
        if(llread(app.fd, receive) < 0){
            printf("APP - Error receiving data packet in applicationLayer.c ...\n");
            return -1;
        }
        int packetDataSize = parseDataPacket((u_int8_t *)receive, bytes);
        
        if(write(fileFd, bytes, packetDataSize) < 0){
            perror("APP - Error writing to file ...\n");
            return -1;
        }
    }

    if(llread(app.fd, receive) < 0){
        printf("APP - Error receiving end control packet in applicationLayer.c ...\n");
        return -1;
    }

    controlStatus = parseControlPacket((u_int8_t *)receive, &fileSize, filename);

    if(controlStatus != END){
        printf("APP - Error receiving end control packet in applicationLayer.c ...\n");
        return -1;
    }

    close(fileFd);
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
