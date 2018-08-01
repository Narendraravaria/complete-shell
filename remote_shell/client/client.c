// sample programs for your use (socket programs)
// this is a sample client code to use

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 10121 /* server port – you need to change this */

int main(int argc, char **argv)      // the argument is the server's IP address 
{
 	int sockfd;
 	struct sockaddr_in servaddr;
 	char sendline[MAXLINE], recvline[MAXLINE];

 	fd_set rfds;
    struct timeval tv;
    int retval;
	
 	//basic check of the arguments
 	//additional checks can be inserted
 	if (argc !=2)
 	{
  		perror("Usage: TCPClient <IP address of the server."); 
  		exit(1);
 	}
	
 	//Create a socket for the client
 	//If sockfd<0 there was an error in the creation of the socket
 	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) 
 	{
  	perror("Problem in creating the socket");
  	exit(2);
 	}
	
 	//Creation of the socket
 	memset(&servaddr, 0, sizeof(servaddr));
 	servaddr.sin_family = AF_INET;
 	servaddr.sin_addr.s_addr= inet_addr(argv[1]);
 	servaddr.sin_port =  htons(SERV_PORT); //convert to big-endian order
	
 	//Connection of the client to the socket 
 	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) 
 	{
  		perror("Problem in connecting to the server");
  		exit(3);
 	}

 	FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
	
	/* Wait up to five seconds. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

	printf("$ ");
	fflush(stdout);

 	while (fgets(sendline, MAXLINE, stdin) != NULL) 
 	{
		
	  	send(sockfd, sendline, strlen(sendline), 0);
	  	retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);
		
		if (retval == -1)
		{
	        perror("select()");
	    }else if (retval)
	    {
	        /* FD_ISSET(0, &rfds) will be true. */
	        if ( FD_ISSET(sockfd, &rfds) )
	        {
	        	if (recv(sockfd, recvline, MAXLINE,0) == 0)
		  		{
		   			//error: server terminated prematurely
			   		// perror("The server terminated prematurely"); 
			   		exit(4);
		  		}
	        }
	    }else
	    {
        	// printf("No data within 2 seconds.\n");
        }

  		tv.tv_sec = 2;
  		tv.tv_usec = 0;

	  	fputs(recvline, stdout);
	  	memset(&recvline[0], 0, sizeof(recvline));

	  	printf("$ ");
		fflush(stdout);
 	}
 	send(sockfd, "exit\n", strlen(sendline), 0);
 	close(sockfd);
 	exit(0);
}


