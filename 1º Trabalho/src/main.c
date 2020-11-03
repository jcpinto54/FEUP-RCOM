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

int main(int argc, char *argv[])
{
    system("umask 0077");
    if (argc == 3) {
        if ((strcmp("-r", argv[1]) != 0) || ((strcmp(SERIAL_PORT_1, argv[2]) != 0) && (strcmp(SERIAL_PORT_2, argv[2]) != 0)) ) {
            printf("Receiver usage: %s -r <port>\nTransmitter usage: %s -s <port> <filename>\n", argv[0], argv[0]);
            exit(1);
        }
    }
    else if (argc == 4) {
        if ((strcmp("-s", argv[1]) != 0) || ((strcmp(SERIAL_PORT_1, argv[2]) != 0) && (strcmp(SERIAL_PORT_2, argv[2]) != 0)) ) {
            printf("Receiver usage: %s -r <port>\nTransmitter usage: %s -s <port> <filename>\n", argv[0], argv[0]);
            exit(1);
        }
    }
    else {
        printf("Receiver usage: %s -r\nTransmitter usage: %s -s <filename>\n", argv[0], argv[0]);
        exit(1);
    }

    if (strcmp("-s", argv[1])== 0) app.status = TRANSMITTER;
    else if (strcmp("-r", argv[1])== 0) app.status = RECEIVER;
    
    if (app.status == TRANSMITTER) {
        strcpy(app.port, argv[2]);
        strcpy(app.filename, argv[3]);
    }
    else if (app.status == RECEIVER) {
        strcpy(app.port, argv[2]);
    }

    appRun();

    return 0;
}

