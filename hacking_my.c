#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>

#include "hacking_my.h"

int sendString(int sockfd, unsigned char *buffer)
{
    int sentBytes, bytesToSend;
    bytesToSend = strlen((char*)buffer);

    while(bytesToSend > 0)
    {
        sentBytes = send(sockfd, buffer, bytesToSend, 0);

        // return 0 on error
        if(sentBytes == -1)
            return 0;

        bytesToSend -= sentBytes;
        buffer += sentBytes;
    }

    return 1;
}

int recvLine(int sockfd, unsigned char *destBuffer)
{
#define EOL "\r\n"
#define EOL_SIZE 2

    unsigned char *ptr;
    int eolMatched = 0;

    ptr = destBuffer;

    while(recv(sockfd, ptr, 1, 0) == 1)
    {
        if(*ptr == EOL[eolMatched])
        {
            eolMatched++;

            if(eolMatched == EOL_SIZE)
            {
                *(ptr - EOL_SIZE + 1) = '\0';
                return strlen((char*)destBuffer);
            }
        }

        else
            eolMatched = 0;

        ptr++;
    }

    // no end of line character found
    return 0;
}

void dump(const char* dataBuffer, const long long length)
{
    unsigned int printLocation = 0;
    char byte;

    while(printLocation <= length)
    {
        for(int i = 0; i < 15; i++)
        {
            if(printLocation + i < length)
                printf("%02x ", (unsigned char)dataBuffer[printLocation + i]);

            else
                printf("   ");
        }

        printf(" | ");

        for(int i = 0; i < 15; i++)
        {
            if(printLocation + i < length)
            {
                byte = dataBuffer[printLocation + i];

                if(byte > 31 && byte < 127)
                    printf("%c ", byte);

                else
                    printf(",");
            }

            else
            {
                printf("\n");
                break;
            }
        }

        printf("\n");
        printLocation += 16;
    }
}

void dump_to_file(const unsigned char* dataBuffer, const unsigned int length, FILE* outputFilePtr)
{
    unsigned int printLocation = 0;
    char byte;

    while(printLocation <= length)
    {
        for(int i = 0; i < 16; i++)
        {
            if(printLocation + i < length)
                fprintf(outputFilePtr, "%02x ", dataBuffer[printLocation + i]);

            else
                fprintf(outputFilePtr, "   ");
        }

        fprintf(outputFilePtr, " | ");

        for(int i = 0; i < 15; i++)
        {
            if(printLocation + i < length)
            {
                byte = dataBuffer[printLocation + i];

                if(byte > 31 && byte < 127)
                    fprintf(outputFilePtr, "%c ", byte);

                else
                    fprintf(outputFilePtr, ",");
            }

            else
            {
                fprintf(outputFilePtr, "\n");
                break;
            }
        }

        fprintf(outputFilePtr, "\n");
        printLocation += 16;
    }
}

void hex_dump_only(const unsigned char* databuffer, const unsigned int length, FILE* outputFilePtr)
{
    unsigned int printLocation = 0;

    while(printLocation <= length)
    {
        fprintf(outputFilePtr, "%02x", databuffer[printLocation]);
        printLocation++;
    }

    fprintf(outputFilePtr, "\n");
}

void fatal(char *message)
{
    char error_message[ERROR_MESSAGE_SIZE];

    strcpy(error_message, "[!!] Fatal Error ");
    strncat(error_message, message, ERROR_MESSAGE_SIZE - 17);
    perror(error_message);
    exit(-1);
}
