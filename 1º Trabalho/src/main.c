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

applicationLayer app;

int main(int argc, char *argv[])
{

    if ((argc != 2) ||
        ((strcmp("-r", argv[1]) != 0) && (strcmp("-s", argv[1]) != 0))) {
        printf("Usage:\t %s -r/-s\n\t\tex: %s -s\n", argv[0], argv[0]);
        exit(1);
    }
    
    if (strcmp("-s", argv[1])== 0) app.status = TRANSMITTER;
    else if (strcmp("-r", argv[1])== 0) app.status = RECEIVER;
    
    char port[11];
    if (app.status == TRANSMITTER) strcpy(port, SERIAL_PORT_1);
    else if (app.status == RECEIVER) strcpy(port, SERIAL_PORT_2);

    // llopen and llclose testing   
    // int llopenReturn = llopen(port, app.status);
    // printf("LLOPEN RETURN: %d\n", llopenReturn);

    // int clearReturn = clearSerialPort(port);
    // printf("CLEAR RETURN : %d\n", clearReturn);

    // if (llopenReturn < 0 || clearReturn) 
    //     exit(0);
    
    // int llcloseReturn = llclose(app.fd);
    // printf("LLCLOSE RETURN: %d\n", llcloseReturn);
    
    printf("LLOPEN RETURN: %d\n", llopen(port, app.status));
    printf("CLEAR RETURN : %d\n", clearSerialPort(port));

    frame_t f;
    u_int8_t iBytes[6];
    u_int8_t dataBytes[5];
    dataBytes[0] = 0x01;
    dataBytes[1] = 0x03;
    dataBytes[2] = 0x04;
    dataBytes[3] = 0x01;
    dataBytes[4] = 0x02;
    iBytes[0] = FLAG;
    iBytes[1] = TRANSMITTER_TO_RECEIVER;
    iBytes[2] = I;
    iBytes[3] = 0;
    iBytes[4] = 0;
    iBytes[5] = FLAG;
    f.bytes = iBytes;
    f.dataSize = 5;
    f.data = dataBytes;
    f.size = 6;
    

    printf("LLCLOSE RETURN: %d\n", llclose(app.fd));
}

