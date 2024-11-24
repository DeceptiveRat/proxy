#include "socket_functions.h"
#include "hacking_my.h"

int main(void)
{
	FILE* outputFilePtr = 0;
	outputFilePtr = fopen("dataExchange.log", "w");
	if(outputFilePtr == NULL)
		fatal("opening file");

	handleConnection();

    return 0;
}
