#include "common.h"

/* sockets */
int buildUDPunicast(int port)
{
    int sock;
    int opt = 1;
    struct sockaddr_in address;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) exit(-1);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) exit(-1);

    struct sockaddr *connectionAddress = (struct sockaddr *) &address;
    int connectionSize = sizeof(address);

    if (bind(sock, connectionAddress, connectionSize) < 0) exit(-1);

    return sock;
}

/* sockets de broadcast */
int buildUDPbroadcast(int port)
{
    int sock;
    int opt = 1;
    struct sockaddr_in address;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) exit(-1);
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &opt, sizeof(opt)) == -1) exit(-1);

    struct sockaddr *connectionAddress = (struct sockaddr *) &address;
    int connectionSize = sizeof(address);

    if (bind(sock, connectionAddress, connectionSize) < 0) exit(-1);

    return sock;
}

/* recuperando mensagem da string */
int getMessageType(char *message)
{
    char* flag = strtok(message, " ");

    if (flag == NULL) return 0;
    return atoi(flag);
}