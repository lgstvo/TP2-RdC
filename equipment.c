#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

/* This section is responsible to just read the commands from the console.*/


void getMessage(char *message)
{
    fflush(stdin);
    scanf("%[^\n]%*c", message);
    return;
}

int socketIpV4(char const *ip, char const *port_str)
{
    int sock = 0;
    struct sockaddr_in server_address;
    int domain = AF_INET;

    if (inet_pton(AF_INET, ip, &server_address.sin_addr) <= 0 ){
        printf("error inet_pton.\n");
        exit(-1);
    }
    
    int port = atoi(port_str);

    if ((sock = socket(domain, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("create socket error.\n");
        exit(-1);
    }

    server_address.sin_family = domain;
    server_address.sin_port = htons(port);

    struct sockaddr *connection_address = (struct sockaddr *)&server_address;
    int connection_size = sizeof(server_address);

    if (connect(sock, connection_address, connection_size) < 0)
    {
        printf("connection error.\n");
        exit(-1);
    }

    return sock;
}

int socketIpV6(char const *ip, char const *port_str)
{
    int sock = 0;
    struct sockaddr_in6 server_address_v6;
    int domain = AF_INET6;

    if (inet_pton(AF_INET6, ip, &server_address_v6.sin6_addr) <= 0 ){
        printf("error inet_pton.\n");
        exit(-1);
    }

    int port = atoi(port_str);

    if ((sock = socket(domain, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("create socket error.\n");
        exit(-1);
    }

    server_address_v6.sin6_family = domain;
    server_address_v6.sin6_port = htons(port);

    struct sockaddr *connection_address = (struct sockaddr *)&server_address_v6;
    int connection_size = sizeof(server_address_v6);

    if (connect(sock, connection_address, connection_size) < 0)
    {
        printf("connection error.\n");
        exit(-1);
    }

    return sock;
}

int getIpVersion(char const *ipstring)
{
    int ipstring_len = strlen(ipstring);
    for(int i = 0; i <= ipstring_len; i++)
    {
        if (ipstring[i] == '.')
        {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        printf("Expected 3 inputs.\n");
        exit(-1);
    }
    int sock = getIpVersion(argv[1]) ? socketIpV6(argv[1], argv[2]) : socketIpV4(argv[1], argv[2]);
    char buffer[500] = {0};
    char message[500];

    for (;;)
    {
        getMessage(message);
        int total_bytes_sent = send(sock, message, strlen(message), 0);
        if (total_bytes_sent <= 0)
        {
            exit(-1);
        }
        memset(buffer, 0, sizeof(buffer));
        int total_bytes_read = read(sock, buffer, 500);
        if (total_bytes_read <= 0)
        {
            exit(-1);
        }
        printf("%s\n", buffer);
    }
    return 0; // never reached
}