#include "common.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/* GLOBALS */
int Socket;
int broadcastSocket;
int nClients = 0;
int eqIDcounter = 1;
int eqID_list[15] = {0};
struct sockaddr_in eq_Address[15] = {0};
socklen_t clientLength = sizeof(struct sockaddr_in);
struct sockaddr_in broadcastAddress;

void *ThreadMain(void *arg);
struct ThreadArguments
{
    char buffer[BUFFER_SIZE];
    socklen_t clientLength;
    struct sockaddr_in clientConnection;
};

void buildMessage(char *buffer, int id, int source, int destination, int payload)
{
    memset(buffer, 0, 100);
    char aux[100] = "";
    if (id)
    {
        sprintf(aux, "%d ", id);
        strcat(buffer, aux);
    }
    if (source)
    {
        sprintf(aux, "%d ", source);
        strcat(buffer, aux);
    }
    if (destination)
    {
        sprintf(aux, "%d ", destination);
        strcat(buffer, aux);
    }
    if (payload)
    {
        sprintf(aux, "%d ", payload);
        strcat(buffer, aux);
    }
    strcat(buffer, "\n");
}

void processREQADD(char *response, struct sockaddr_in connection)
{
    int flag = 0;
    if (nClients == 15)
        buildMessage(response, 7, 0, 0, 4);
    else
    {
        flag = 1;
        eqID_list[nClients] = eqIDcounter;
        eq_Address[nClients] = connection;
        nClients++;
        eqIDcounter++;

        printf("Equipment %d added\n", eqIDcounter - 1);
        buildMessage(response, 3, 0, 0, eqIDcounter - 1);
    }
    int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLength);
    if (bytesSent < 1)
        exit(-1);
    bytesSent = sendto(broadcastSocket, response, strlen(response), 0, (struct sockaddr *)&broadcastAddress, clientLength);
    if (bytesSent < 1)
        exit(-1);
    if(flag)
    {
        char msg[100] = "4 ";
        for(int i = 0; i < 15; i++)
        {
            if(eqID_list[i])
            {
                char idstr[100] = "";
                sprintf(idstr, "%d ", eqID_list[i]);
                strcat(msg, idstr);
            }
        }
        strcat(msg, "\n");
        int bytesSent = sendto(Socket, msg, strlen(msg), 0, (struct sockaddr *)&connection, clientLength);
        if (bytesSent < 1)
            exit(-1);
    }
}

void processREQREM(char *response, struct sockaddr_in connection)
{
    char *idMsgString = strtok(NULL, " ");
    if (idMsgString == NULL)
        return;
    int idMsg = atoi(idMsgString);

    int found = 0;
    int found_pos = -1;
    for (int i = 0; i < 15; i++)
    {
        if (eqID_list[i] == idMsg)
        {
            found = 1;
            found_pos = i;
            break;
        }
    }
    if (!found)
    {
        buildMessage(response, 7, 0, 0, 1);
    }
    else
    {
        eqID_list[found_pos] = 0;
        memset(&eq_Address[found_pos], 0, sizeof(eq_Address[found_pos]));

        buildMessage(response, 8, 0, idMsg, 1);
        printf("Equipment %d removed\n", idMsg);
    }
    int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLength);
    if (bytesSent < 1)
        exit(-1);
    buildMessage(response, 2, idMsg, 0, 0);
    bytesSent = sendto(broadcastSocket, response, strlen(response), 0, (struct sockaddr *)&broadcastAddress, clientLength);
    if (bytesSent < 1)
        exit(-1);
}

void processREQINF_RESINF(char *message, char *response, struct sockaddr_in connection)
{
    char message_copy[100];
    strcpy(message_copy, message);
    char *eq1str = strtok(NULL, " ");
    char *eq2str = strtok(NULL, " ");
    if (eq1str == NULL)
        return;
    if (eq2str == NULL)
        return;

    int eq1 = atoi(eq1str);
    int eq2 = atoi(eq2str);

    int found1 = 0;
    int found2 = 0;
    int found2_pos = -1;
    for (int i = 0; i < 15; i++)
    {
        if (eqID_list[i] == eq1)
            found1 = 1;
        if (eqID_list[i] == eq2)
        {
            found2 = 1;
            found2_pos = i;
        }
    }
    if (!found1)
    {
        buildMessage(response, 7, 0, 0, 2);
        int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLength);
        if (bytesSent < 1)
            exit(-1);
    }
    else if (!found2)
    {
        buildMessage(response, 7, 0, 0, 3);
        int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLength);
        if (bytesSent < 1)
            exit(-1);
    }
    else
    {
        strcpy(response, message_copy);
        int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&eq_Address[found2_pos], clientLength);
        if (bytesSent < 1)
            exit(-1);
    }
}

void unicastCommandSwitch(int messageType, char *message, char *response, struct sockaddr_in connection)
{
    switch (messageType)
    {
    case REQADD:
        processREQADD(response, connection);
        break;
    case REQREM:
        processREQREM(response, connection);
        break;
    case REQINF:
        processREQINF_RESINF(message, response, connection);
        break;
    case RESINF:
        processREQINF_RESINF(message, response, connection);
        break;
    }
}

void *unicastThread(void *args)
{
    struct ThreadArguments *threadArgs = (struct ThreadArguments *)args;
    char buffer[BUFFER_SIZE] = "";
    char response[100] = "";
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recvfrom(Socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&threadArgs->clientConnection, &threadArgs->clientLength);
        if (bytesReceived < 1)
            exit(-1);
        char buffer_copy[100];
        strcpy(buffer_copy, buffer);
        int messageType = getMessageType(buffer_copy);

        unicastCommandSwitch(messageType, buffer, response, threadArgs->clientConnection);
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
        exit(-1);

    pthread_t thread_unicast;
    char *port = strdup(argv[1]);
    int portNumber = atoi(port);
    Socket = buildUDPunicast(portNumber);
    broadcastSocket = buildUDPbroadcast(0);
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons((in_port_t)(7777));
    broadcastAddress.sin_addr.s_addr = INADDR_BROADCAST;

    struct ThreadArguments *threadArgs = (struct ThreadArguments *)malloc(sizeof(struct ThreadArguments));
    threadArgs->clientLength = 16;

    int threadStatus = pthread_create(&thread_unicast, NULL, unicastThread, (void *)threadArgs);
    if (threadStatus)
        exit(-1);
    pthread_join(thread_unicast, NULL);

    return 0;
}