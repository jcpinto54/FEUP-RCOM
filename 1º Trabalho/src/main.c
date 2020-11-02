#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dataLayer.h"
#include "macros.h"
#include "utils.h"

// // TEST PREPAREI
#include "dataLayerPrivate.h"


applicationLayer application;

int main(int argc, char *argv[])
{

    if ((argc != 2) ||
        ((strcmp("-r", argv[1]) != 0) && (strcmp("-s", argv[1]) != 0))) {
        printf("Usage:\t %s -r/-s\n\t\tex: %s -s\n", argv[0], argv[0]);
        exit(1);
    }
    
    if (strcmp("-s", argv[1])== 0) application.status = TRANSMITTER;
    else if (strcmp("-r", argv[1])== 0) application.status = RECEIVER;
    
    if (application.status == TRANSMITTER) strcpy(application.port, SERIAL_PORT_1);
    else if (application.status == RECEIVER) strcpy(application.port, SERIAL_PORT_2);



    // TEST DATA LAYER
    if (llopen(application.port, application.status) < 0) {
        printf("error in llopen\n"); 
        clearSerialPort(application.port);
        exit(1);
    }

    switch (application.status) {
        case TRANSMITTER:;
            llwrite(application.fd, "ola eu sou o joao", 17);
        break;
        case RECEIVER:;
            char *received;
            llread(application.fd, &received);
            printf("Data Received: %s\n", received);
        break;
    }

    int llcloseReturn = llclose(application.fd);
    if (llcloseReturn < 0) {
        printf("error in llclose\n"); 
        clearSerialPort(application.port);
        exit(1);
    }

    return 0;
}

