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

    if (argc == 2) {
        if ((strcmp("-r", argv[1]) != 0)) {
            printf("Receiver usage: %s -r\nTransmitter usage: %s -s <filename>\n", argv[0], argv[0]);
            exit(1);
        }
    }
    else if (argc == 3) {
        if ((strcmp("-s", argv[1]) != 0)) {
            printf("Receiver usage: %s -r\nTransmitter usage: %s -s <filename>\n", argv[0], argv[0]);
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
        strcpy(app.port, SERIAL_PORT_1);
        strcpy(app.filename, argv[2]);
    }
    else if (app.status == RECEIVER) {
        strcpy(app.port, SERIAL_PORT_2);
    }

    appRun();

    return 0;
}

