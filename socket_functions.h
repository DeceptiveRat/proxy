#pragma once

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#define NAME_LENGTH 30
#define FILE_NAME_LENGTH 40
#define BUFFER_SIZE 4096
#define DESTINATION_NAME_LENGTH 100
#define DESTINATION_PORT_LENGTH 5
#define PORT "7890"
#define MAX_CONNECTION_COUNT 1
#define CONNECTION_ESTABLISHED_MESSAGE_LENGTH 39
#define DOMAIN_NAME_LENGTH 6
#define DOMAIN_NAME_COUNT 7
#define DOMAIN_NAME_FILE_NAME "domains.txt"

struct threadParameters
{
	int *socket;
	int connectionID;
	int* writeBufferSize;
	int* readBufferSize;
	unsigned char* writeBuffer;
	unsigned char* readBuffer;
	bool* shutDown;
	FILE* localOutputFilePtr;
	FILE* globalOutputFilePtr;
	char connectedTo[NAME_LENGTH+1];
	// TODO: mutex lock isn't needed. writing writeBufferSize only happens when the size is 0. writing readBufferSize only happens when the size is not 0. Change later and see if it still works
	pthread_mutex_t *mutex_writeBufferSize;
	pthread_mutex_t *mutex_readBufferSize;
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
	unsigned char dataFromClient[BUFFER_SIZE+1];
	unsigned char dataFromServer[BUFFER_SIZE+1];
	bool shutDown;
	pthread_mutex_t mutex_dataFromClientSize;
	pthread_mutex_t mutex_dataFromServerSize;
	struct threadParameters serverArgs;
	struct threadParameters clientArgs;
	pthread_t serverThread;
	pthread_t clientThread;
	FILE* outputFilePtr;
};

void setDomainNames();
void setupConnectionResources(struct connectionResources* connections, int connectionCount, FILE* globalOutputFilePtr);
void handleConnection();
int returnListeningSocket();
int returnSocketToClient(const int listeningSocket);
int getDestinationName(const unsigned char* receivedData, char* destinationNameBuffer, FILE* outputFilePtr);
int getDestinationPort(const unsigned char* destinationNameEnd, char* destinationPortBuffer, const bool isHTTPS, FILE* outputFilePtr);
struct addrinfo returnDestinationAddressInfo(const char* destinationName, const char* destinationPort, FILE* outputFilePtr);
int returnSocketToServer(const struct addrinfo destinationAddressInformation);
bool isConnectMethod(const unsigned char* receivedData); 
bool isNumber(const char* stringToCheck);
void* threadFunction(void* args);
void* listeningThreadFunction(void* args);
void cleanupConnections(struct connectionResources *conRes, int connectionCount);
