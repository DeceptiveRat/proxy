#pragma once

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>

#define NAME_LENGTH 7
#define FILE_NAME_LENGTH 40
#define BUFFER_SIZE 4096
#define DESTINATION_NAME_LENGTH 100
#define DESTINATION_PORT_LENGTH 5
#define PORT "7890"
#define MAX_CONNECTION_COUNT 11
#define CONNECTION_ESTABLISHED_MESSAGE_LENGTH 39
#define DOMAIN_NAME_LENGTH 6
#define DOMAIN_NAME_COUNT 7
#define DOMAIN_NAME_FILE_NAME "domains.txt"
#define CACHE_SIZE 1024
#define OBJECT_NAME_LENGTH 100
#define CACHE_METADATA_FILE_NAME "cache.dat"
#define CACHE_FILE_NAME "cache.txt"
#define SERVER_TIMEOUT_VALUE 2
#define CONNECTION_TIMEOUT_VALUE 2
#define TIMEOUT_COUNT 20000

struct threadParameters
{
	// connection info
	int *socket;
	int connectionID;
	char connectedTo[NAME_LENGTH+1];
	bool* shutDown;
	bool isHTTPS;

	// read/write buffer info
	int* writeBufferSize;
	int* readBufferSize;
	unsigned char* writeBuffer;
	unsigned char* readBuffer;

	// file pointers
	FILE* localOutputFilePtr;
	FILE* globalOutputFilePtr;

	// mutex locks
	// TODO: mutex lock isn't needed. writing writeBufferSize only happens when the size is 0. writing readBufferSize only happens when the size is not 0. Change later and see if it still works
	pthread_mutex_t *mutex_writeBufferSize;
	pthread_mutex_t *mutex_readBufferSize;

	// cache info
	struct requestCache* cachePtr;
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
	// connection info
	int clientSocket;
	int serverSocket;
	bool shutDown;
	
	// buffer info
	int dataFromClientSize;
	int dataFromServerSize;
	unsigned char dataFromClient[BUFFER_SIZE+1];
	unsigned char dataFromServer[BUFFER_SIZE+1];

	// mutex locks
	pthread_mutex_t mutex_dataFromClientSize;
	pthread_mutex_t mutex_dataFromServerSize;

	// thread info
	pthread_t serverThread;
	pthread_t clientThread;
	struct threadParameters serverArgs;
	struct threadParameters clientArgs;

	// file pointers
	FILE* outputFilePtr;
};

// TODO change length of the value later
struct requestCache
{
	char objectName[OBJECT_NAME_LENGTH];
	int objectNameLength;
	int valueLength;
	unsigned char value[BUFFER_SIZE*20];
};

// setup functions
void setDomainNames();
void setupConnectionResources(struct connectionResources* connections, int connectionCount, FILE* globalOutputFilePtr, struct requestCache* cachePtr);
pthread_mutex_t* setupMutexes();

// action function
void handleConnection();
void saveToCache(struct requestCache *cachePtr, const unsigned char* dataToCache, int dataLength);
int isInCache(struct requestCache *cachePtr, const char* requestedObject, unsigned char** copyDestination);
int saveCacheToFile(struct requestCache *cachePtr);
int readCacheFromFile(struct requestCache *cachePtr);
void handle_alarm(int sig);

// return sockets
int returnListeningSocket();
int returnSocketToClient(const int listeningSocket);
int returnSocketToServer(const struct addrinfo destinationAddressInformation);

// get information
int getDestinationName(const unsigned char* receivedData, char* destinationNameBuffer, FILE* outputFilePtr);
int getDestinationPort(const unsigned char* destinationNameEnd, char* destinationPortBuffer, const bool isHTTPS, FILE* outputFilePtr);
struct addrinfo returnDestinationAddressInfo(const char* destinationName, const char* destinationPort, FILE* outputFilePtr);
bool isConnectMethod(const unsigned char* receivedData); 
bool isNumber(const char* stringToCheck);
int getHashValue(const char* requestedObject);
void getRequestedObject(const unsigned char* requestMessage, char* requestedObject);

// thread functions
void* threadFunction(void* args);
void* listeningThreadFunction(void* args);

// clean up functions
void cleanupConnections(struct connectionResources *conRes, int connectionCount);
void cleanMutexes(pthread_mutex_t* mutexes);
