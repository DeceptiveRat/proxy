#define _GNU_SOURCE
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

pthread_mutex_t mutex_outputFile = PTHREAD_MUTEX_INITIALIZER;

void handleConnection() // #handleConnection
{
	// initialize local variable
	char connectionEstablishedResponse[CONNECTION_ESTABLISHED_MESSAGE_LENGTH + 1] = "HTTP/1.1 200 Connection Established\r\n\r\n\0";
	FILE* outputFilePtr = 0;
	outputFilePtr = fopen("dataExchange.log", "w");
	if(outputFilePtr == NULL)
		fatal("opening file");
	static pthread_mutex_t mutexes[MAX_CONNECTION_COUNT*2] = {
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_MUTEX_INITIALIZER
	};
	struct connectionResources conResources[MAX_CONNECTION_COUNT];
	for(int i = 0;i<MAX_CONNECTION_COUNT;i++)
	{
		conResources[i].mutex_dataFromClient = mutexes[i*2];
		conResources[i].mutex_dataFromServer = mutexes[i*2 + 1];
	}
	for(int i = 0;i<MAX_CONNECTION_COUNT;i++)
		conResources[i].shutDown = false;
	int connectionCount = 0;

	// initialize variables for listening thread
	pthread_t listeningThread;
    int hostSocket = returnListeningSocket();
	int acceptedSocket;
	bool acceptedSocketPending;
	bool shutDownListeningSocket;
	pthread_mutex_t mutex_acceptedSocket = PTHREAD_MUTEX_INITIALIZER;
	struct listeningThreadParameters listeningThreadArgs;

	listeningThreadArgs.listeningSocket = hostSocket;
	listeningThreadArgs.acceptedSocket = &acceptedSocket;
	listeningThreadArgs.acceptedSocketPending = &acceptedSocketPending;
	listeningThreadArgs.shutDown = &shutDownListeningSocket;
	listeningThreadArgs.mutex_acceptedSocket = &mutex_acceptedSocket;

    // create listening thread
	pthread_create(&listeningThread, NULL, listeningThreadFunction, &listeningThreadArgs);

	while(!shutDownListeningSocket)
	{
		// there is a new connection pending
		if(acceptedSocketPending && connectionCount <MAX_CONNECTION_COUNT)
		{
			pthread_mutex_lock(&mutex_outputFile);
			printf("[   main   ] new connection made\n");
			fprintf(outputFilePtr, "[   main   ] new connection made\n");
			pthread_mutex_unlock(&mutex_outputFile);

			struct connectionResources temp;
			pthread_mutex_lock(&mutex_acceptedSocket);
			temp.clientSocket = acceptedSocket;
			acceptedSocketPending = false;
			pthread_mutex_unlock(&mutex_acceptedSocket);
			temp.dataFromClient[0] = '\0';
			temp.dataFromServer[0] = '\0';
			int receiveLength = recv(temp.clientSocket, temp.dataFromClient, BUFFER_SIZE, 0);
			if(receiveLength == -1)
			{
				// clean up code
				for(int i = 0;i<MAX_CONNECTION_COUNT;i++)
					conResources[i].shutDown = true;
				for(int i = 0;i<MAX_CONNECTION_COUNT*2;i++)
					pthread_mutex_destroy(&mutexes[i]);
				cleanupConnections(conResources, MAX_CONNECTION_COUNT);
				pthread_mutex_destroy(&mutex_outputFile);
				fclose(outputFilePtr);
				fatal("receiving from client");
			}
			pthread_mutex_lock(&mutex_outputFile);
			printf("[   main   ] received %d bytes from client %d\n", receiveLength, connectionCount);
			dump(temp.dataFromClient, receiveLength);
			pthread_mutex_unlock(&mutex_outputFile);
			temp.dataFromClientSize = receiveLength;

			// send automatic response if HTTPS connection
			bool isHTTPS = isConnectMethod(temp.dataFromClient);

			// get information about server
			char destinationName[DESTINATION_NAME_LENGTH + 1];
			int nameOffset = getDestinationName(temp.dataFromClient, destinationName);
			char destinationPort[DESTINATION_PORT_LENGTH + 1];
			getDestinationPort(temp.dataFromClient+nameOffset, destinationPort, isHTTPS);
			struct addrinfo destinationAddressInformation = returnDestinationAddressInfo(destinationName, destinationPort);

			// create socket to destination
			temp.serverSocket = returnSocketToServer(destinationAddressInformation);
			pthread_mutex_lock(&mutex_outputFile);
			printf("[   main   ] established TCP connection with server\n");
			fprintf(outputFilePtr, "[   main   ] established TCP connection with server\n");
			pthread_mutex_unlock(&mutex_outputFile);

			if(isHTTPS)
			{
				if(sendString(temp.clientSocket, connectionEstablishedResponse, CONNECTION_ESTABLISHED_MESSAGE_LENGTH) == 0)
				{
					// clean up code
					for(int i = 0;i<MAX_CONNECTION_COUNT;i++)
						conResources[i].shutDown = true;
					for(int i = 0;i<MAX_CONNECTION_COUNT*2;i++)
						pthread_mutex_destroy(&mutexes[i]);
					cleanupConnections(conResources, MAX_CONNECTION_COUNT);
					pthread_mutex_destroy(&mutex_outputFile);
					fclose(outputFilePtr);
					fatal("sending 200 connection established");
				}
				else
				{
					pthread_mutex_lock(&mutex_outputFile);
					printf("[   main   ] Sent 200 connection established\n");
					fprintf(outputFilePtr, "[   main   ] Sent 200 connection established\n");
					pthread_mutex_unlock(&mutex_outputFile);
					temp.dataFromClientSize = 0;
				}
			}

			temp.shutDown = false;
			conResources[connectionCount] = temp;

			// set up parameters for the server and client
			conResources[connectionCount].serverArgs.socket = conResources[connectionCount].serverSocket;
			conResources[connectionCount].serverArgs.writeBufferItemSize = &conResources[connectionCount].dataFromServerSize;
			conResources[connectionCount].serverArgs.readBufferItemSize = &conResources[connectionCount].dataFromClientSize;
			conResources[connectionCount].serverArgs.writeBuffer = conResources[connectionCount].dataFromServer;
			conResources[connectionCount].serverArgs.readBuffer = conResources[connectionCount].dataFromClient;
			conResources[connectionCount].serverArgs.shutDown = &(conResources[connectionCount].shutDown);
			conResources[connectionCount].serverArgs.outputFilePtr = outputFilePtr;
			strncpy(conResources[connectionCount].serverArgs.connectedTo, "server\0", 7);
			conResources[connectionCount].serverArgs.mutex_writeBuffer = &(conResources[connectionCount].mutex_dataFromServer);
			conResources[connectionCount].serverArgs.mutex_readBuffer = &(conResources[connectionCount].mutex_dataFromClient);
			conResources[connectionCount].serverArgs.connectionID = connectionCount;

			conResources[connectionCount].clientArgs.socket = conResources[connectionCount].clientSocket;
			conResources[connectionCount].clientArgs.writeBufferItemSize = &conResources[connectionCount].dataFromClientSize;
			conResources[connectionCount].clientArgs.readBufferItemSize = &conResources[connectionCount].dataFromServerSize;
			conResources[connectionCount].clientArgs.writeBuffer = conResources[connectionCount].dataFromClient;
			conResources[connectionCount].clientArgs.readBuffer = conResources[connectionCount].dataFromServer;
			conResources[connectionCount].clientArgs.shutDown = &(conResources[connectionCount].shutDown);
			conResources[connectionCount].clientArgs.outputFilePtr = outputFilePtr;
			strncpy(conResources[connectionCount].clientArgs.connectedTo, "client\0", 7);
			conResources[connectionCount].clientArgs.mutex_writeBuffer = &(conResources[connectionCount].mutex_dataFromClient);
			conResources[connectionCount].clientArgs.mutex_readBuffer = &(conResources[connectionCount].mutex_dataFromServer);
			conResources[connectionCount].clientArgs.connectionID = connectionCount;

			// create threads
			pthread_create(&(conResources[connectionCount].clientThread), NULL, threadFunction, &(conResources[connectionCount].clientArgs));
			pthread_create(&(conResources[connectionCount].serverThread), NULL, threadFunction, &conResources[connectionCount].serverArgs);

			connectionCount++;
		}
	}

	// clean up code
	for(int i = 0;i<MAX_CONNECTION_COUNT;i++)
		conResources[i].shutDown = true;
	for(int i = 0;i<MAX_CONNECTION_COUNT*2;i++)
		pthread_mutex_destroy(&mutexes[i]);
	cleanupConnections(conResources, MAX_CONNECTION_COUNT);
	pthread_mutex_destroy(&mutex_outputFile);
	fclose(outputFilePtr);
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

/* create, connect, and return a socket to the client */
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

void* threadFunction(void* args) // #threadFunction
{
	// set up local variables with argument
	struct threadParameters parameters = *(struct threadParameters*)args;
	int socket = parameters.socket;
	int ID = parameters.connectionID;
	int* readBufferItemSize = parameters.readBufferItemSize;
	int* writeBufferItemSize = parameters.writeBufferItemSize;
	char* writeBuffer = parameters.writeBuffer;
	char* readBuffer = parameters.readBuffer;
	bool* shutDown = parameters.shutDown;
	FILE* outputFilePtr = parameters.outputFilePtr;
	char connectedTo[NAME_LENGTH+1];
	memcpy(connectedTo, parameters.connectedTo, NAME_LENGTH+1);
	pthread_mutex_t *mutex_writeBuffer = parameters.mutex_writeBuffer;
	pthread_mutex_t *mutex_readBuffer = parameters.mutex_readBuffer;

	char tempReadBuffer[BUFFER_SIZE+1];
	ssize_t recvResult;

	while(!(*shutDown))
	{
		recvResult = recv(socket, tempReadBuffer, BUFFER_SIZE, MSG_DONTWAIT);

		if(recvResult == -1)
		{
			// error reading data
			if(errno != EAGAIN && errno != EWOULDBLOCK)
			{
				*shutDown = true;
				printf("[%d - %s] Terminating: Error reading data.\nErrno: %d\n", ID, connectedTo, errno);
				pthread_exit(NULL);
			}
			// no data to read 
			else {}
		}
		// data read
		else
		{
			// debug ---------------------
			if(recvResult == 0)
			{
				*shutDown = true;
				printf("[%d - %s] Terminating: 0 bytes received\n", ID, connectedTo);
				pthread_exit(NULL);
			}

			pthread_mutex_lock(&mutex_outputFile);
			printf("[%d - %s] Read %ld bytes\n", ID, connectedTo, recvResult);
			dump(tempReadBuffer, recvResult);
			fprintf(outputFilePtr, "[%d - %s] Read %ld bytes\n", ID, connectedTo, recvResult);
			dump_to_file(tempReadBuffer, recvResult, outputFilePtr);
			pthread_mutex_unlock(&mutex_outputFile);
			// ----------------------------

			// wait until buffer is empty before writing to it
			while(*writeBufferItemSize != 0) {};

			printf("[%d - %s] attempting to acquire mutex for write buffer...\n", ID, connectedTo);
			pthread_mutex_lock(mutex_writeBuffer);
			printf("[%d - %s] acquired mutex for write buffer...\n", ID, connectedTo);
			memcpy(writeBuffer, tempReadBuffer, recvResult);
			pthread_mutex_unlock(mutex_writeBuffer);
			printf("[%d - %s] unlocked mutex for write buffer...\n", ID, connectedTo);

			// debug ----------------------------------
			*writeBufferItemSize = recvResult;
			pthread_mutex_lock(&mutex_outputFile);
			printf("[%d - %s] Wrote %ld bytes\n", ID, connectedTo, recvResult);
			fprintf(outputFilePtr, "[%d - %s] Wrote %ld bytes\n", ID, connectedTo, recvResult);
			pthread_mutex_unlock(&mutex_outputFile);
		}

		// if there is data in the read buffer, send it
		if(*readBufferItemSize != 0)
		{
			pthread_mutex_lock(&mutex_outputFile);
			printf("[%d - %s] dump before send:\n", ID, connectedTo);
			dump(readBuffer, *readBufferItemSize);
			printf("\n");
			pthread_mutex_unlock(&mutex_outputFile);

			if(sendString(socket, readBuffer, *readBufferItemSize) == 0)
			{
				*shutDown = true;
				printf("[%d - %s] Terminating: error sending data\n", ID, connectedTo);
				pthread_exit(NULL);
			}
			// debug ----------------------
			pthread_mutex_lock(&mutex_outputFile);
			printf("[%d - %s] Sent data\n", ID, connectedTo);
			fprintf(outputFilePtr, "[%d - %s] Sent data\n", ID, connectedTo);
			pthread_mutex_unlock(&mutex_outputFile);
			// ---------------------------

			printf("[%d - %s] attempting to acquire mutex for read buffer...\n", ID, connectedTo);
			pthread_mutex_lock(mutex_readBuffer);
			printf("[%d - %s] acquired mutex for read buffer...\n", ID, connectedTo);
			*readBufferItemSize = 0;
			pthread_mutex_unlock(mutex_readBuffer);
			printf("[%d - %s] unlocked mutex for read buffer...\n", ID, connectedTo);
			
			// debug ---------------------------
			pthread_mutex_lock(&mutex_outputFile);
			printf("[%d - %s] Set buffer to empty\n", ID, connectedTo);
			fprintf(outputFilePtr, "[%d - %s] Set buffer to empty\n", ID, connectedTo);
			pthread_mutex_unlock(&mutex_outputFile);
			// ------------------------------
		}
	}

	// clean up code
	close(socket);
	printf("[%d - %s] Terminating: shutdown variable set\n", ID, connectedTo);
	pthread_exit(NULL);
}

void* listeningThreadFunction(void* args) // #listeningThreadFunction
{
	struct listeningThreadParameters parameter = *(struct listeningThreadParameters*)args;
	int listeningSocket = parameter.listeningSocket;
	int* acceptedSocket = parameter.acceptedSocket;
	bool* acceptedSocketPending = parameter.acceptedSocketPending;
	bool* shutDown = parameter.shutDown;
	pthread_mutex_t* mutex_acceptedSocket = parameter.mutex_acceptedSocket;
	
	int tempAcceptedSocket = 0;
	while(!(*shutDown))
	{
		if(tempAcceptedSocket == 0)
			tempAcceptedSocket = returnSocketToClient(listeningSocket);
		
		pthread_mutex_lock(mutex_acceptedSocket);
		if(!(*acceptedSocketPending))
		{
			*acceptedSocket = tempAcceptedSocket;
			*acceptedSocketPending = true;
			tempAcceptedSocket = 0;
		}
		pthread_mutex_unlock(mutex_acceptedSocket);

	}

	if(tempAcceptedSocket != 0)
		close(tempAcceptedSocket);
	pthread_mutex_lock(mutex_acceptedSocket);
	if(!(*acceptedSocketPending))
		close(*acceptedSocket);
	pthread_mutex_unlock(mutex_acceptedSocket);
	close(listeningSocket);
	pthread_exit(NULL);
}

bool cleanupConnections(struct connectionResources* conRes, int connectionCount) // #cleanupConnections
{
	void* retval;
	int result;

	for(int i = 0;i<connectionCount;i++)
	{
		// TODO: add code to check for errors later
		result = pthread_tryjoin_np(conRes[i].clientThread, &retval);
		if(result == EBUSY)
			pthread_join(conRes[i].clientThread, NULL);
		result = pthread_tryjoin_np(conRes[i].serverThread, &retval);
		if(result == EBUSY)
			pthread_join(conRes[i].serverThread, NULL);
	}
}
