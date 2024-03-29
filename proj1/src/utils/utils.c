#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "utils.h"

int ceiling(float x) {
    int y = x;
    if (y == x) return x;
    return x + 1;
}

u_int64_t bit(unsigned n) {
    return 1 << n;
}

u_int8_t getBit(int byte, int b)
{
    return (byte >> b) & bit(0);
}


void printString(char *str)
{
    printf("\nStarting printString...\n\tSize: %ld\n", strlen(str));
    for (int i = 0; i < strlen(str); i++)
    {
        printf("\tstr[%d]: %c\n", i, str[i]);
    }
    printf("printString ended\n");
}


void displayStats(struct timeval begin, struct timeval end) {
    printf("\n\nPROGRAM STATS:\nExecution in seconds: %lf\n", (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)/1000000.0);
}

