#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#include "socket_functions.h"
#include "hacking_my.h"

#define BUFFER_SIZE 4096
#define DESTINATION_NAME_LENGTH 100
#define DESTINATION_PORT_LENGTH 5
#define PORT "7890"

pthread_mutex_t mutex_clientBuffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_serverBuffer = PTHREAD_MUTEX_INITIALIZER;

void handleConnection() // #handleConnection
{
	FILE* outputFilePtr = 0;
	outputFilePtr = fopen("dataExchange.log", "w");
	if(outputFilePtr == NULL)
		fatal("opening file");

	bool isHTTPS;
    // create listening socket and socket to client
    int hostSocket = returnListeningSocket();
    int socketToClient = returnSocketToClient(hostSocket);

    // print data from client ------------------------------------------------
    char clientData[BUFFER_SIZE+1];
	memset(clientData, '\0', BUFFER_SIZE);
    int receiveLength = recv(socketToClient, &clientData, BUFFER_SIZE, 0);

    if(receiveLength == -1)
        fatal("receiving from client");

    printf("Received %d bytes from client\n", receiveLength);
    dump(clientData, receiveLength);
	isHTTPS = isConnectMethod(clientData);

    char destinationName[DESTINATION_NAME_LENGTH + 1];
    int nameOffset = getDestinationName(clientData, destinationName);

    // find destination port number -------------------------------------
    char destinationPort[DESTINATION_PORT_LENGTH + 1];
	getDestinationPort(clientData+nameOffset, destinationPort, isHTTPS);

    // get destination information
    struct addrinfo destinationAddressInformation = returnDestinationAddressInfo(destinationName, destinationPort);

    // create socket to destination
    int socketToDestination = returnSocketToServer(destinationAddressInformation);

	if(isHTTPS)
	{
		char connectionEstablishedResponse[36] = "HTTP/1.1 200 Connection Established\0";
		if(sendString(socketToClient, connectionEstablishedResponse) == 0)
			fatal("sending 200 connection established");
	}

	char destinationData[BUFFER_SIZE+1];
	destinationData[0] = '\0';

	bool terminateThreads = false;
	struct threadParameters args;
	args.clientSocket = socketToClient;
	args.serverSocket = socketToDestination;
	args.clientDataBuffer = clientData;
	args.serverDataBuffer = destinationData;
	args.shutDown = &terminateThreads;
	args.outputFilePtr = outputFilePtr;

	pthread_t serverThread, clientThread;
	pthread_create(&clientThread, NULL, clientFunction, &args);
	pthread_create(&serverThread, NULL, serverFunction, &args);

	pthread_join(clientThread, NULL);
	pthread_join(serverThread, NULL);

	printf("Value of errno: %d\n", errno);

	fclose(outputFilePtr);
    close(socketToDestination);
    close(socketToClient);
    close(hostSocket);
}

/* create, bind, and return a listening socket */
int returnListeningSocket() // #returnListeningSocket
{
    struct addrinfo hostAddrHint, *hostResult;
    int hostSocket;

    memset(&hostAddrHint, 0, sizeof(struct addrinfo));
    hostAddrHint.ai_family = AF_INET;
    hostAddrHint.ai_socktype = SOCK_STREAM;
    hostAddrHint.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hostAddrHint, &hostResult);
    hostSocket = socket(hostResult->ai_family, hostResult->ai_socktype, hostResult->ai_protocol);

    if(hostSocket == -1)
        fatal("creating host socket");

    int yes = 1;

    if(setsockopt(hostSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
        fatal("setsockopt");

    if(bind(hostSocket, hostResult->ai_addr, hostResult->ai_addrlen) == -1)
        fatal("binding socket");

    if(listen(hostSocket, 10) == -1)
        fatal("listening on socket");

    freeaddrinfo(hostResult);
    return hostSocket;
}

/* create, bind, and return a socket to the client */
int returnSocketToClient(const int listeningSocket) // #returnSocketToClient
{
    struct sockaddr clientAddress;
    socklen_t sin_size = sizeof(struct sockaddr);
    int socketToClient = accept(listeningSocket, &clientAddress, &sin_size);

    if(socketToClient == -1)
        fatal("accepting connection");

    char clientAddressString[INET_ADDRSTRLEN];
    struct sockaddr_in clientAddress_in = *((struct sockaddr_in*)&clientAddress);

    if(inet_ntop(AF_INET, &clientAddress_in.sin_addr, clientAddressString, INET_ADDRSTRLEN) == NULL)
        fatal("converting client address to string");

    printf("got connection from %s port %d\n", clientAddressString, clientAddress_in.sin_port);

    return socketToClient;
}

/* extract the destination name string from the HTTP request */
/* returns offset of name from start of data */
int getDestinationName(const char* receivedData, char* destinationNameBuffer) // #getDestinationName
{
    char *destinationNameStart, *destinationNameEnd;
    int destinationNameLength;

    destinationNameStart = strstr(receivedData, "Host: ");
	if(destinationNameStart == NULL)
		fatal("finding host");

    destinationNameStart += 6;
    destinationNameEnd = strstr(destinationNameStart, ".com");
	if(destinationNameEnd == NULL)
		fatal("finding host name end");
    destinationNameEnd += 4;
    destinationNameLength = destinationNameEnd - destinationNameStart;

    strncpy(destinationNameBuffer, destinationNameStart, destinationNameLength);
    destinationNameBuffer[destinationNameLength] = '\0';

    printf("destination name is: %s\n", destinationNameBuffer);
	return receivedData - destinationNameStart; 
}

void getDestinationPort(const char* destinationNameEnd, char* destinationPortBuffer, const bool isHTTPS) // #getDestinationPort
{
	if(*destinationNameEnd == ':')
	{
		char* destinationPortEnd = strstr(destinationNameEnd, "\r\n");
		if(destinationPortEnd == NULL)
			fatal("finding port number");

		int destinationPortLength = destinationPortEnd - destinationNameEnd - 1;
		strncpy(destinationPortBuffer, destinationNameEnd + 1, destinationPortLength);
		destinationPortBuffer[destinationPortLength] = '\0';

		if(!isNumber(destinationPortBuffer))
			fatal("finding port number");
	}
	if(isHTTPS)
		strncpy(destinationPortBuffer, "443\0", 4);
	else
		strncpy(destinationPortBuffer, "80\0", 3);
	
    printf("destination port is: %s\n", destinationPortBuffer);
}

/* get additional information about the destination */
struct addrinfo returnDestinationAddressInfo(const char* destinationName, const char* destinationPort) // #returnDestinationAddressInfo
{
    struct addrinfo destinationAddressHint, *destinationAddressResult;
    memset(&destinationAddressHint, 0, sizeof(struct addrinfo));
    destinationAddressHint.ai_family = AF_INET;
    destinationAddressHint.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(destinationName, destinationPort, &destinationAddressHint, &destinationAddressResult) != 0)
        fatal("getting address information for the destination");

    char destinationAddressString[INET_ADDRSTRLEN];
    struct sockaddr_in destinationAddress_in = *(struct sockaddr_in*)destinationAddressResult->ai_addr;

    if(inet_ntop(AF_INET, &destinationAddress_in.sin_addr, destinationAddressString, INET_ADDRSTRLEN) == NULL)
        fatal("converting destination ip address to string");

    printf("destination ip address: %s\n", destinationAddressString);

    struct addrinfo destinationAddrInfo = *destinationAddressResult;
    freeaddrinfo(destinationAddressResult);

    return destinationAddrInfo;
}

/* create, connect, and return a socket to the server */
int returnSocketToServer(const struct addrinfo destinationAddressInformation) // #returnSocketToServer
{
    int socketToDestination;
    socketToDestination = socket(destinationAddressInformation.ai_family, destinationAddressInformation.ai_socktype, destinationAddressInformation.ai_protocol);

    if(socketToDestination == -1)
        fatal("creating socket to server");

    if(connect(socketToDestination, destinationAddressInformation.ai_addr, destinationAddressInformation.ai_addrlen) == -1)
        fatal("connecting to server");

    return socketToDestination;
}

bool isConnectMethod(const char* receivedData) // #isConnectMethod
{
	if(strstr(receivedData, "CONNECT ") == NULL)
		return false;
	else
		return true;
}

bool isNumber(const char* stringToCheck) // #isNumber
{
	int stringLength = strlen(stringToCheck);
	for(int i = 0;i<stringLength;i++)
	{
		if(stringToCheck[i] >= 48 && stringToCheck[i] <= 57)
			continue;
		else
			return false;
	}

	return true;
}

void* clientFunction(void *args) // #clientFunction
{
	struct threadParameters parameters = *(struct threadParameters*)args;
	int clientSocket = parameters.clientSocket;
	char* clientDataBuffer = parameters.clientDataBuffer;
	char* serverDataBuffer = parameters.serverDataBuffer;
	bool* shutDown = parameters.shutDown;
	FILE* outputFilePtr = parameters.outputFilePtr;

	char tempReadBuffer[BUFFER_SIZE+1];
	ssize_t recvResult;
	while(!(*shutDown))
	{
		recvResult = recv(clientSocket, tempReadBuffer, BUFFER_SIZE, MSG_DONTWAIT);

		if(recvResult == -1)
		{
			// error reading data
			if(errno != EAGAIN && errno != EWOULDBLOCK)
			{
				*shutDown = true;
				printf("Errno: %d\n", errno);
				pthread_exit(NULL);
			}
			// no data to read
			else{}
		}
		// data read
		else
		{
			printf("Read %ld bytes from client:\n", recvResult);
			dump(tempReadBuffer, recvResult);
			if(recvResult != 0)
			{
				fprintf(outputFilePtr, "Read %ld bytes from client:\n", recvResult);
				dump_to_file(tempReadBuffer, recvResult, outputFilePtr);
			}
			// wait until buffer is empty (has been read by the server function
			while(clientDataBuffer[0] != '\0')

			pthread_mutex_lock(&mutex_clientBuffer);
			strncpy(clientDataBuffer, tempReadBuffer, recvResult);
			pthread_mutex_unlock(&mutex_clientBuffer);
		}

		// if there is data from the server, forward it
		if(serverDataBuffer[0] != '\0')
		{
			if(sendString(clientSocket, serverDataBuffer) == 0)
			{
				*shutDown = true;
				pthread_exit(NULL);
			}
			printf("Sent data from server buffer to client\n");
			fprintf(outputFilePtr, "Sent data from server buffer to client\n");
			pthread_mutex_lock(&mutex_serverBuffer);
			serverDataBuffer[0] = '\0';
			pthread_mutex_unlock(&mutex_serverBuffer);
			printf("Set server buffer to empty\n");
			fprintf(outputFilePtr, "Set server buffer to empty\n");
		}
	}

	// clean up code
	pthread_exit(NULL);
}

void* serverFunction(void* args) // #clientFunction
{
	struct threadParameters parameters = *(struct threadParameters*)args;
	int serverSocket = parameters.serverSocket;
	char* clientDataBuffer = parameters.clientDataBuffer;
	char* serverDataBuffer = parameters.serverDataBuffer;
	bool* shutDown = parameters.shutDown;
	FILE* outputFilePtr = parameters.outputFilePtr;

	char tempReadBuffer[BUFFER_SIZE+1];
	ssize_t recvResult;
	while(!(*shutDown))
	{
		recvResult = recv(serverSocket, tempReadBuffer, BUFFER_SIZE, MSG_DONTWAIT);

		if(recvResult == -1)
		{
			// error reading data
			if(errno != EAGAIN && errno != EWOULDBLOCK)
			{
				*shutDown = true;
				printf("Errno: %d\n", errno);
				pthread_exit(NULL);
			}
			// no data to read 
			else {}
		}
		// data read
		else
		{
			printf("Read %ld bytes from server:\n", recvResult);
			dump(tempReadBuffer, recvResult);
			if(recvResult != 0)
			{
				fprintf(outputFilePtr, "Read %ld bytes from server:\n", recvResult);
				dump_to_file(tempReadBuffer, recvResult, outputFilePtr);
			}
			// wait until buffer is empty (has been read by the client function
			while(serverDataBuffer[0] != '\0')

			pthread_mutex_lock(&mutex_serverBuffer);
			strncpy(serverDataBuffer, tempReadBuffer, recvResult);
			pthread_mutex_unlock(&mutex_serverBuffer);
		}

		// if there is data from the client, forward it
		if(clientDataBuffer[0] != '\0')
		{
			if(sendString(serverSocket, clientDataBuffer) == 0)
			{
				*shutDown = true;
				pthread_exit(NULL);
			}
			printf("Sent data from client buffer to server\n");
			fprintf(outputFilePtr, "Sent data from client buffer to server\n");
			pthread_mutex_lock(&mutex_clientBuffer);
			clientDataBuffer[0] = '\0';
			pthread_mutex_unlock(&mutex_clientBuffer);
			printf("Set client buffer to empty\n");
			fprintf(outputFilePtr, "Set client buffer to empty\n");
		}
	}

	// clean up code
	pthread_exit(NULL);
}
