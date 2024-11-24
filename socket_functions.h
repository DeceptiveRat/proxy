#pragma once

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#define NAME_LENGTH 30
#define BUFFER_SIZE 4096
#define DESTINATION_NAME_LENGTH 100
#define DESTINATION_PORT_LENGTH 5
#define PORT "7890"
#define MAX_CONNECTION_COUNT 3
#define CONNECTION_ESTABLISHED_MESSAGE_LENGTH 39

struct threadParameters
{
	int socket;
	int connectionID;
	int* writeBufferItemSize;
	int* readBufferItemSize;
	char* writeBuffer;
	char* readBuffer;
	bool* shutDown;
	FILE* outputFilePtr;
	char connectedTo[NAME_LENGTH+1];
	// TODO: get rid of mutex since the thread can only write to 1 buffer and 1 buffer size
	pthread_mutex_t *mutex_writeBuffer;
	pthread_mutex_t *mutex_readBuffer;
};

struct listeningThreadParameters
{
	int listeningSocket;
	int* acceptedSocket;
	bool* acceptedSocketPending;
	bool* shutDown;
	// encloses both acceptedSocketPending and acceptedSocket
	pthread_mutex_t *mutex_acceptedSocket;
};

struct connectionResources
{
	int clientSocket;
	int serverSocket;
	int dataFromClientSize;
	int dataFromServerSize;
	char dataFromClient[BUFFER_SIZE+1];
	char dataFromServer[BUFFER_SIZE+1];
	bool shutDown;
	pthread_mutex_t mutex_dataFromClient;
	pthread_mutex_t mutex_dataFromServer;
	struct threadParameters serverArgs;
	struct threadParameters clientArgs;
	pthread_t serverThread;
	pthread_t clientThread;
};

void handleConnection();
int returnListeningSocket();
int returnSocketToClient(const int listeningSocket);
int getDestinationName(const char* receivedData, char* destinationNameBuffer);
void getDestinationPort(const char* destinationNameEnd, char* destinationPortBuffer, const bool isHTTPS);
struct addrinfo returnDestinationAddressInfo(const char* destinationName, const char* destinationPort);
int returnSocketToServer(const struct addrinfo destinationAddressInformation);
bool isConnectMethod(const char* receivedData); 
bool isNumber(const char* stringToCheck);
void* threadFunction(void* args);
void* listeningThreadFunction(void* args);
bool cleanupConnections(struct connectionResources *conRes, int connectionCount);
