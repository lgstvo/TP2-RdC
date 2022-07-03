#ifndef COMMON
#define COMMON

#define BUFFER_SIZE 100

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

enum{
    NOTFOUND = 1,
    SNOTFOUND,
    TNOTFOUND,
    LIMITEXCEEDED,
};

void validadeArguments(int argc, int minArgs);
int buildUDPunicast(int port);
int buildUDPbroadcast(int port);

int getMessageType(char *message);

#endif