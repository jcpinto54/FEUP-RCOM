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

    
    // // TEST LLOPEN AND LLCLOSE
    // int llopenReturn = llopen(port, application.status);
    // printf("LLOPEN RETURN: %d\n", llopenReturn);
    // if (llopenReturn < 0) 
    //     exit(1);
    // int llcloseReturn = llclose(application.fd);
    // printf("LLCLOSE RETURN: %d\n", llcloseReturn);
    

    // // TEST PREPAREI
    // frame_t **iFrames;
    // char data[10];
    // data[0] = 0x11;
    // data[1] = 0x10;
    // data[2] = 0x10;
    // data[3] = 0x10;
    // data[4] = 0x10;
    // data[5] = 0x10;
    // data[6] = 0x10;
    // data[7] = 0x10;
    // data[8] = 0x10;
    // data[9] = 0x10;
    // int fNeeded = prepareI(data, 10, &iFrames);
    // for (int i = 0; i < fNeeded; i++) printFrame(iFrames[i]);


    if (llopen(application.port, application.status) < 0) {
        perror("error in llopen"); 
        clearSerialPort(application.port);
        exit(1);
    }

    switch (application.status) {
        case TRANSMITTER:;
            llwrite(application.fd, "ola eu sou o joao", 17);
            printf("\n\nNOT SUPPOSED TO SEE THIS! (for now)\n\n");
        break;
        case RECEIVER:;
            frame_t frame;
            receiveIMessage(&frame);
        break;
    }

    if (llclose(application.fd) < 0) {
        perror("error in llclose"); 
        clearSerialPort(application.port);
        exit(1);
    }

    return 0;

}

