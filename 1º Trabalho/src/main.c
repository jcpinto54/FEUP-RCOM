#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "applicationLayer.h"
#include "dataLayer.h"
#include "macros.h"
#include "utils.h"

// // TEST PREPAREI
// #include "dataLayerPrivate.h"

extern application app;

int main(int argc, char *argv[])
{

    if ((argc != 2) ||
        ((strcmp("-r", argv[1]) != 0) && (strcmp("-s", argv[1]) != 0))) {
        printf("Usage:\t %s -r/-s\n\t\tex: %s -s\n", argv[0], argv[0]);
        exit(1);
    }
    
    if (strcmp("-s", argv[1])== 0) app.status = TRANSMITTER;
    else if (strcmp("-r", argv[1])== 0) app.status = RECEIVER;
    
    if (app.status == TRANSMITTER) strcpy(app.port, SERIAL_PORT_1);
    else if (app.status == RECEIVER) strcpy(app.port, SERIAL_PORT_2);

    appRun();

    return 0;
}

