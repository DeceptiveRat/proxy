#pragma once

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>

#define ERROR_MESSAGE_SIZE 100

// ------------------------------------------------------ #structures -------------------------------------------------------------- //

// ------------------------------------------------------- #functions -------------------------------------------------------------- //
/*
 * Accepts a socket FD and a ptr to a null terminated string to send.
 * It will make sure all byets are sent.
 * Returns 0 on error and 1 on success.
 */
int sendString(int sockfd, char *buffer);

/*
 * Accepts a socket FD and a ptr to a destination.
 * Receives from the socket until EOL byte sequence is detected.
 * Returns the size of the line read or 0 if not found.
 */
int recvLine(int sockfd, unsigned char *destBuffer);

/*
 * Dump dataBuffer to:
 * dump -> stdout
 * dump_to_file -> file 
 * hex_dump_only -> file
 */
void dump(const char* dataBuffer, const long long length);
void dump_to_file(const unsigned char* dataBuffer, const unsigned int length, FILE* outputFilePtr);
void hex_dump_only(const unsigned char* databuffer, const unsigned int length, FILE* outputFilePtr);

void fatal(char* message);

// ------------------------------------------------------- #variables -------------------------------------------------------------- //

