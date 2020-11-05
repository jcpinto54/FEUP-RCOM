#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "appLayer/applicationLayer.h"
#include "dataLayer/dataLayer.h"
#include "macros.h"
#include "utils/utils.h"

application app;
int baudrate = BAUDRATE;
unsigned maxFrameSize = MAX_FRAME_SIZE;
unsigned maxFrameDataLength = MAX_FRAME_DATA_LENGTH;
unsigned maxPacketLength = MAX_PACKET_LENGTH;
unsigned maxPacketDataLength = MAX_PACKET_DATA_LENGTH;

int main(int argc, char *argv[])
{
    system("umask 0077");

    if (argc == 3 || argc == 5) {
        if ((strcmp("-r", argv[1]) != 0) || ((strcmp(SERIAL_PORT_1, argv[2]) != 0) && (strcmp(SERIAL_PORT_2, argv[2]) != 0)) ) {
            printf("Receiver usage: %s -r <port> [baudrate maxFrameSize]\nTransmitter usage: %s -s <port> <filename> [baudrate maxFrameSize]\n", argv[0], argv[0]);
            exit(1);
        }
    }
    else if (argc == 4 || argc == 6) {
        if ((strcmp("-s", argv[1]) != 0) || ((strcmp(SERIAL_PORT_1, argv[2]) != 0) && (strcmp(SERIAL_PORT_2, argv[2]) != 0)) ) {
            printf("Receiver usage: %s -r <port> [baudrate maxFrameSize]\nTransmitter usage: %s -s <port> <filename> [baudrate maxFrameSize]\n", argv[0], argv[0]);
            exit(1);
        }
    }
    else {
        printf("Receiver usage: %s -r <port> [baudrate maxFrameSize]\nTransmitter usage: %s -s <port> <filename> [baudrate maxFrameSize]\n", argv[0], argv[0]);
        exit(1);
    }

    if (strcmp("-s", argv[1])== 0) {
        app.status = TRANSMITTER;
        if(argc == 6) {
            baudrate = atoi(argv[4]);
            maxFrameSize = atoi(argv[5]);
            maxFrameDataLength = (maxFrameSize - 8)/2;
            maxPacketLength = maxFrameDataLength;
            maxPacketDataLength = maxFrameDataLength - 4;
        }
        printf("Baudrate: %d\n", baudrate);
        printf("maxFrameSize: %d\n", maxFrameSize);
    }
    else if (strcmp("-r", argv[1])== 0) {
        app.status = RECEIVER;
        if(argc == 5) {
            baudrate = atoi(argv[3]);
            maxFrameSize = atoi(argv[4]);
            maxFrameDataLength = (maxFrameSize - 8)/2;
            maxPacketLength = maxFrameDataLength;
            maxPacketDataLength = maxFrameDataLength - 4;
        }
        printf("Baudrate: %d\n", baudrate);
        printf("maxFrameSize: %d\n", maxFrameSize);
    }
    
    if (app.status == TRANSMITTER) {
        strcpy(app.port, argv[2]);
        strcpy(app.filename, argv[3]);
        // int fd = open(app.filename, O_RDONLY);
        // char bytes[5];
        // read(fd, bytes, 4);
        // printf("bytes: %s\n", bytes);
    }
    else if (app.status == RECEIVER) {
        strcpy(app.port, argv[2]);
    }

    appRun();

    return 0;
}

