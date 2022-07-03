#ifndef COMMON
#define COMMON

#define ADDR_LEN 16
#define BUFFER_SIZE_BYTES 100
#define MAX_CLIENTS 15

#define SERVER_INPUT_ARGS 2
#define SERVER_MAX_THREADS 15
#define SERVER_MAX_PENDING 5


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

void validadeArguments(int argc, int minArgs);
int buildUDPunicast(int port);
int buildUDPbroadcast(int port);

int getMessageType(char *message);

#endif