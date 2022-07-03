#ifndef COMMON
#define COMMON

#define BUFFER_SIZE 100

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* protocol enumeration */
enum{
    REQADD = 1,
    REQREM,
    RESADD,
    RESLIST,
    REQINF,
    RESINF,
    ERROR,
    OK,
};

/* error enumeration */
enum{
    NOTFOUND = 1,
    SNOTFOUND,
    TNOTFOUND,
    LIMITEXCEEDED,
};

int buildUDPunicast(int port);
int buildUDPbroadcast(int port);

int getMessageType(char *message);

#endif