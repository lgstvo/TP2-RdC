#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "common.h"

/* GLOBALS */
int eqID = 0;
int equipments[15];
int clientSocket, clientBroadcastSocket;
socklen_t clientLength = sizeof(struct sockaddr_in);

struct ThreadArguments
{
    struct sockaddr_in serverAddr;
};

/* */
void sendREQADD(struct sockaddr_in serverAddr)
{
    char buffer[BUFFER_SIZE] = "1";
    int bytesSent = sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, clientLength);
    if (bytesSent < 1)
        exit(-1);
}

void sendCLOSECONNECTION(struct sockaddr_in serverAddr)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "2 %d\n", eqID);
    int bytesSent = sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, clientLength);
    if (bytesSent < 1)
        exit(-1);
}

void sendREQINF(struct sockaddr_in serverAddr, int destinationID)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "5 %d %d\n", eqID, destinationID);
    int bytesSent = sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, clientLength);
    if (bytesSent < 1)
        exit(-1);
}

void listEquipment()
{
    for(int i = 0; i < 15; i++)
    {
        if(equipments[i])
            printf("%d ", equipments[i]);
    }
    printf("\n");
}

int nonBlockRead(char *message)
{
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    FD_ZERO(&readfds);
    FD_SET(1, &readfds);

    select(2, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(1, &readfds))
    {
        read(1, message, BUFFER_SIZE - 1);
        return 1;
    }
    else
        fflush(stdout);
    return 0;
}

void *readInputThread(void *args)
{
    struct ThreadArguments *threadArgs = (struct ThreadArguments *)args;
    char buffer[BUFFER_SIZE] = "";

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (nonBlockRead(buffer))
        {
            if(buffer[0] == 'l')
            {
                listEquipment();
            }
            else if(buffer[0] == 'c')
            {
                sendCLOSECONNECTION(threadArgs->serverAddr);
            }
            else if(buffer[0] == 'r')
            {
                char message_copy[100];
                strcpy(message_copy, buffer);
                strtok(message_copy, " ");
                strtok(NULL, " ");
                strtok(NULL, " ");
                char* destIDstr = strtok(NULL, " ");
                if(destIDstr == NULL)
                    break;
                int destID = atoi(destIDstr);
                sendREQINF(threadArgs->serverAddr, destID);
            }

            int bytesSent = sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&threadArgs->serverAddr, clientLength);
            if (bytesSent < 1)
                exit(-1);
        }
    }
    return NULL;
}

void processRESADD()
{
    char *aux = strtok(NULL, " ");
    if(aux == NULL)
        return;
    eqID = atoi(aux);
    printf("New ID: %d\n", eqID);
}

void processRESLIST()
{
    int i = 0;
    char *buffer;
    while ((buffer = strtok(NULL, " ")) != NULL)
    {
        equipments[i] = atoi(buffer);
        i++;
    }
}

void processREQINF(struct sockaddr_in serverAddr)
{
    char* aux;
    aux = strtok(NULL, " ");
    if (aux == NULL)
        return;
    int originID = atoi(aux);
    aux = strtok(NULL, " ");
    if (aux == NULL)
        return;
    int destinationID = atoi(aux);

    char buffer[50];
    char main_buffer[500];
    sprintf(buffer, "6 %d %d ", destinationID, originID);
    sprintf(main_buffer, "%s%d.%d%d\n", buffer, rand() % 9, rand() % 9, rand() % 9);
    printf("requested information\n");

    int bytesSent = sendto(clientSocket, main_buffer, strlen(main_buffer), 0, (struct sockaddr *)&serverAddr, clientLength);
    if (bytesSent < 1)
        exit(-1);
    
}

void processRESINF()
{
    char* aux;
    aux = strtok(NULL, " ");
    if (aux == NULL)
        return;
    int originID = atoi(aux);
    strtok(NULL, " ");
    
    char* payload = strtok(NULL, " ");
    printf("Value from %d: %s\n", originID, payload);
}

void processERROR()
{
    char* aux = strtok(NULL, " ");
    if (aux == NULL)
        return;
    int errorCode = atoi(aux);
    switch (errorCode)
    {
        case NOTFOUND:
            printf("Equipment not found\n");
            break;
        case SNOTFOUND:
            printf("Source equipment not found\n");
            break;
        case TNOTFOUND:
            printf("Target equipment not found\n");
            break;
        case LIMITEXCEEDED:
            printf("Equipment limit exceeded\n");
            exit(-1);
    }
}

void processOK()
{
    printf("Successful removal\n");
    exit(0);
}

void commandSwitch(int messageType, struct sockaddr_in serverAddr)
{
    switch (messageType)
    {
        case RESADD:
            processRESADD();
            break;
        case RESLIST:
            processRESLIST();
            break;
        case REQINF:
            processREQINF(serverAddr);
            break;
        case RESINF:
            processRESINF();
            break;
        case ERROR:
            processERROR();  
            break;
        case OK:
            processOK();
            break;
    }
}

void *readThread(void *args)
{
    struct ThreadArguments *threadArgs = (struct ThreadArguments *)args;
    char buffer[BUFFER_SIZE] = "";
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&threadArgs->serverAddr, &clientLength);
        if (bytesReceived < 1)
            exit(-1);
        char buffer_copy[100];
        strcpy(buffer_copy, buffer);
        int messageType = getMessageType(buffer_copy);
        commandSwitch(messageType, threadArgs->serverAddr);
    }
}

void processBroadcastRESADD()
{
    char *aux = strtok(NULL, " ");
    if(aux == NULL)
        return;
    eqID = atoi(aux);
    for(int i = 0; i < 15; i++)
    {
        if(equipments[i] == eqID)
            return;
        if(equipments[i] == 0)
        {
            equipments[i] = eqID;
            printf("Equipment %d added\n", eqID);
            return;
        }
    }
}

void processBreadcastREQREM()
{
    char *aux = strtok(NULL, " ");
    if(aux == NULL)
        return;
    eqID = atoi(aux);
    for(int i = 0; i < 15; i++)
    {
        if(equipments[i] == eqID)
        {
            equipments[i] = 0;
            printf("Equipment %d removed\n", eqID);
            return;
        }
    }
}

void commandBroadcastSwitch(int messageType)
{
    switch (messageType)
    {
        case RESADD:
            processBroadcastRESADD();
            break;
        case REQREM:
            processBreadcastREQREM();
            break;
    }
}

void *readBroadcastThread(void *args)
{
    struct ThreadArguments *threadArgs = (struct ThreadArguments *)args;
    char buffer[BUFFER_SIZE] = "";
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recvfrom(clientBroadcastSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&threadArgs->serverAddr, &clientLength);
        if (bytesReceived < 1)
            exit(-1);
        char buffer_copy[100];
        strcpy(buffer_copy, buffer);
        int messageType = getMessageType(buffer_copy);
        //printf("%s\n", buffer_copy);
        commandBroadcastSwitch(messageType);
    }
}

int main(int argc, char const *argv[])
{
    srand(time(0));

    if (argc < 3)
        exit(-1);

    char *serverIP = strdup(argv[1]);
    char *serverPort = strdup(argv[2]);
    int serverPortNumber = atoi(serverPort);
    clientSocket = buildUDPunicast(0);
    clientBroadcastSocket = buildUDPunicast(7777);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPortNumber);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);

    sendREQADD(serverAddr);

    
    struct sockaddr_in broadcastServerAddr;
    broadcastServerAddr.sin_family = AF_INET;
    broadcastServerAddr.sin_port = htons(7777);
    broadcastServerAddr.sin_addr.s_addr = inet_addr(serverIP);
    

    pthread_t receiveThread;
    struct ThreadArguments *receiveThreadArgs = (struct ThreadArguments *)malloc(sizeof(struct ThreadArguments));
    receiveThreadArgs->serverAddr = serverAddr;
    int receiveThreadStatus = pthread_create(&receiveThread, NULL, readThread, (void *)receiveThreadArgs);
    if (receiveThreadStatus != 0)
        exit(-1);

    
    pthread_t receiveBroadcastThread;
    struct ThreadArguments *receiveBroadcastThreadArgs = (struct ThreadArguments *)malloc(sizeof(struct ThreadArguments));
    receiveBroadcastThreadArgs->serverAddr = broadcastServerAddr;
    int receiveBroadcastThreadStatus = pthread_create(&receiveBroadcastThread, NULL, readBroadcastThread, (void *)receiveBroadcastThreadArgs);
    if (receiveBroadcastThreadStatus != 0) exit(-1);

    pthread_t sendThread;
    struct ThreadArguments *sendThreadArgs = (struct ThreadArguments *)malloc(sizeof(struct ThreadArguments));
    sendThreadArgs->serverAddr = serverAddr;
    int sendThreadStatus = pthread_create(&sendThread, NULL, readInputThread, (void *)sendThreadArgs);
    if (sendThreadStatus != 0)
        exit(-1);

    pthread_join(receiveThread, NULL);
    // pthread_join(receiveBroadcastThread, NULL);
    pthread_join(sendThread, NULL);

    return 0; // never reached
}