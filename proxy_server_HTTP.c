#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "hacking_my.h"

#define BUFFER_SIZE 4096
#define DESTINATION_NAME_LENGTH 100
#define DESTINATION_PORT_LENGTH 6
#define PORT "7890"

int main(void)
{
	int functionResult = 0;

    // set up listening socket -----------------------------------
    struct addrinfo host_addr_hint, *host_result;
    int host_socket;

    memset(&host_addr_hint, 0, sizeof(struct addrinfo));
    host_addr_hint.ai_family = AF_INET;
    host_addr_hint.ai_socktype = SOCK_STREAM;
    host_addr_hint.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &host_addr_hint, &host_result);
    host_socket = socket(host_result->ai_family, host_result->ai_socktype, host_result->ai_protocol);
	if(host_socket == -1)
		fatal("creating host socket");

    if(bind(host_socket, host_result->ai_addr, host_result->ai_addrlen) == -1)
        fatal("binding socket");

    if(listen(host_socket, 10) == -1)
        fatal("listening on socket");

    // create socket to client -------------------------------------------
    struct sockaddr client_address;
    socklen_t sin_size = sizeof(struct sockaddr);
    int socket_to_client = accept(host_socket, &client_address, &sin_size);

    if(socket_to_client == -1)
        fatal("accepting connection");

    char client_address_string[INET_ADDRSTRLEN];
    struct sockaddr_in clientAddress_in = *((struct sockaddr_in*)&client_address);

    if(inet_ntop(AF_INET, &clientAddress_in.sin_addr, client_address_string, INET_ADDRSTRLEN) == NULL)
        fatal("converting client address to string");

    printf("got connection from %s port %d\n", client_address_string, clientAddress_in.sin_port);

	// print data from client ------------------------------------------------
	unsigned char receivedData[BUFFER_SIZE];
	int receiveLength = recv(socket_to_client, &receivedData, BUFFER_SIZE, 0);
	if(receiveLength == -1)
		fatal("receiving from client");
	printf("Received %d bytes from client\n", receiveLength);
	dump(receivedData, receiveLength);

	// find destination name -----------------------------------------------------
	char destinationName[DESTINATION_NAME_LENGTH];
	char *destinationNameStart, *destinationNameEnd;
	destinationNameStart = strstr(receivedData, "Host: ");
	destinationNameStart += 6;
	destinationNameEnd = strstr(destinationNameStart, ".com");
	destinationNameEnd += 4;
	int destinationNameLength = destinationNameEnd - destinationNameStart;
	strncpy(destinationName, destinationNameStart, destinationNameLength);
	destinationName[destinationNameLength] = '\0';
	printf("destination name is: %s\n", destinationName);

	// find destination port number -------------------------------------
	char destinationPort[DESTINATION_PORT_LENGTH];
	/*
	char* destinationPortEnd = strstr(destinationNameEnd, "\r\n");
	int destinationPortLength = destinationPortEnd - destinationNameEnd - 1;
	strncpy(destinationPort, destinationNameEnd + 1, destinationPortLength);
	destinationPort[destinationPortLength] = '\0';
	printf("destination port is: %s\n", destinationPort);
	*/
	strncpy(destinationPort, "80\0", 3);

	// get destination address --------------------------------------------
	struct addrinfo destinationAddressHint, *destinationAddressResult;
	memset(&destinationAddressHint, 0, sizeof(struct addrinfo));
	destinationAddressHint.ai_family = AF_INET;
	destinationAddressHint.ai_socktype = SOCK_STREAM;

	if(getaddrinfo(destinationName, destinationPort, &destinationAddressHint, &destinationAddressResult) !=0)
		fatal("getting address information for the destination");

	char destinationAddressString[INET_ADDRSTRLEN];
	struct sockaddr_in destinationAddress_in = *(struct sockaddr_in*)destinationAddressResult->ai_addr;
	if(inet_ntop(AF_INET, &destinationAddress_in.sin_addr, destinationAddressString, INET_ADDRSTRLEN) == NULL)
		fatal("converting destination ip address to string");

	printf("destination ip address: %s\n", destinationAddressString);

	// create a socket to the server ---------------------------
	int socket_to_destination;
	socket_to_destination = socket(destinationAddressResult->ai_family, destinationAddressResult->ai_socktype, destinationAddressResult->ai_protocol);
	if(socket_to_destination == -1)
		fatal("creating socket to server");
	
	if(connect(socket_to_destination, destinationAddressResult->ai_addr, destinationAddressResult->ai_addrlen) == -1)
		fatal("connecting to server");
	
	if(sendString(socket_to_destination, receivedData) == 0)
		fatal("sending data to server");
	
	receiveLength = recv(socket_to_destination, &receivedData, BUFFER_SIZE, 0);
	if(receiveLength == -1)
		fatal("receiving data from server");
	
	printf("Received %d bytes from server\n", receiveLength);
	dump(receivedData, receiveLength);

	if(sendString(socket_to_client, receivedData) == 0)
		fatal("sending data to server");
	
	close(socket_to_destination);
	close(socket_to_client);
	close(host_socket);
    return 0;
}
