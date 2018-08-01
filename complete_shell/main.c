// sample programs for your use (socket programs)
// this is a sample client code to use

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

int socket_redir_flag;

int main()
{
	int SOCKET_FLAG = 0;
	char s[100];

	printf("$ ");
	fflush(stdout);

	while(fgets ( s, sizeof(s), stdin ) != NULL)
	{	
		// Call function and pass input command s with SOCKET_FLAG as argument
		runShellCommand(s, SOCKET_FLAG, 0);

		printf("$ ");
		fflush(stdout);
	}
	printf("\n");
	return 0;
}