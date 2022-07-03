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
int numberOfThreads = 0;
int numberOfClients = 0;
int equipmentIdCounter = 1;
int equipmentsIds[MAX_CLIENTS] = {0};
struct sockaddr_in equipmentsAdresses[MAX_CLIENTS] = {0};
socklen_t clientLen = sizeof(struct sockaddr_in);
struct sockaddr_in broadcastAddr;

void *ThreadMain(void *arg);
struct ThreadArgs
{
    socklen_t clientLen;
    struct sockaddr_in clientCon;
    char buffer[BUFFER_SIZE_BYTES];
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
    if (numberOfClients == MAX_CLIENTS)
        buildMessage(response, 7, 0, 0, 4);
    else
    {
        equipmentsIds[numberOfClients] = equipmentIdCounter;
        equipmentsAdresses[numberOfClients] = connection;
        numberOfClients++;
        equipmentIdCounter++;

        printf("Equipment %d added\n", equipmentIdCounter - 1);
        buildMessage(response, 3, 0, 0, equipmentIdCounter - 1);
    }
    int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLen);
    if (bytesSent < 1)
        exit(-1);
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
        if (equipmentsIds[i] == idMsg)
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
        equipmentsIds[found_pos] = 0;
        memset(&equipmentsAdresses[found_pos], 0, sizeof(equipmentsAdresses[found_pos]));

        buildMessage(response, 8, 0, idMsg, 1);
        printf("Equipment %d removed\n", idMsg);
    }
    int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLen);
    if (bytesSent < 1)
        exit(-1);
}

void processREQINF_RESINF(int messageType, char *message, char *response, struct sockaddr_in connection)
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
        if (equipmentsIds[i] == eq1)
            found1 = 1;
        if (equipmentsIds[i] == eq2)
        {
            found2 = 1;
            found2_pos = i;
        }
    }
    if (!found1)
    {
        buildMessage(response, 7, 0, 0, 2);
        int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLen);
        if (bytesSent < 1)
            exit(-1);
    }
    else if (!found2)
    {
        buildMessage(response, 7, 0, 0, 3);
        int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&connection, clientLen);
        if (bytesSent < 1)
            exit(-1);
    }
    else
    {
        strcpy(response, message_copy);
        int bytesSent = sendto(Socket, response, strlen(response), 0, (struct sockaddr *)&equipmentsAdresses[found2_pos], clientLen);
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
        processREQINF_RESINF(messageType, message, response, connection);
        break;
    case RESINF:
        processREQINF_RESINF(messageType, message, response, connection);
        break;
    }
}

void *unicastThread(void *args)
{
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    char buffer[100] = "";
    char response[100] = "";
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recvfrom(Socket, buffer, BUFFER_SIZE_BYTES, 0, (struct sockaddr *)&threadArgs->clientCon, &threadArgs->clientLen);
        if (bytesReceived < 1)
            exit(-1);
        char buffer_copy[100];
        strcpy(buffer_copy, buffer);
        int messageType = getMessageType(buffer_copy);

        unicastCommandSwitch(messageType, buffer, response, threadArgs->clientCon);
        printf("%d\n", messageType);
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

    struct ThreadArgs *threadArgs = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
    threadArgs->clientLen = ADDR_LEN;

    int threadStatus = pthread_create(&thread_unicast, NULL, unicastThread, (void *)threadArgs);
    if (threadStatus)
        exit(-1);
    pthread_join(thread_unicast, NULL);

    return 0;
}