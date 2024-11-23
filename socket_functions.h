#pragma once

#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

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

struct threadParameters
{
	int clientSocket;
	int serverSocket;
	char* clientDataBuffer;
	char* serverDataBuffer;
	bool* shutDown;
	FILE* outputFilePtr;
};
