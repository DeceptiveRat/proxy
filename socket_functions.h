#pragma once

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#define NAME_LENGTH 30

void handleConnection();
int returnListeningSocket();
int returnSocketToClient(const int listeningSocket);
int getDestinationName(const char* receivedData, char* destinationNameBuffer);
void getDestinationPort(const char* destinationNameEnd, char* destinationPortBuffer, const bool isHTTPS);
struct addrinfo returnDestinationAddressInfo(const char* destinationName, const char* destinationPort);
int returnSocketToServer(const struct addrinfo destinationAddressInformation);
bool isConnectMethod(const char* receivedData); 
bool isNumber(const char* stringToCheck);
void* clientFunction(void* args);
void* serverFunction(void* args);
void* threadFunction(void* args);

struct threadParameters
{
	int clientSocket;
	int serverSocket;
	char* clientDataBuffer;
	char* serverDataBuffer;
	bool* shutDown;
	FILE* outputFilePtr;
};

struct threadParam
{
	int socket;
	char* writeBuffer;
	char* readBuffer;
	bool* shutDown;
	FILE* outputFilePtr;
	char connectedTo[NAME_LENGTH+1];
	pthread_mutex_t *mutex_writeBuffer;
	pthread_mutex_t *mutex_readBuffer;
};
