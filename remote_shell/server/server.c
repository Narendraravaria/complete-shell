// Sample concurrent server code using fork (process)

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/wait.h>
#include <fcntl.h>	// for open,read,write,close system calls
#include <errno.h>
#include <pthread.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 10121 /* server port – you need to change this */
#define LISTENQ 8 /*maximum number of client connections*/

int socket_redir_flag;

char *removeNewlineString(char *s)
{
	int slen = strlen(s);
	if(slen>0 && s[slen-1] == '\n')
	{
		s[slen-1] = '\0';
	}
	return s;
}

void * shell(void *arg)
{
	int connfd, n, n1, fdin;
	char command[MAXLINE], out_buf[MAXLINE];
	pthread_t tid;
	char *output_file = malloc( sizeof(char) * 64 );
	char *remove_file_cmd = malloc( sizeof(char) * 64 );

	printf("SERVER: thread %u created for dealing with client requests\n",  (unsigned int)tid,command);
	sprintf(output_file, "output_%u",(unsigned int)tid);
	
	tid = pthread_self();
  	connfd = *((int *) arg);
    free(arg);

  	while ( (n = recv(connfd, command, MAXLINE,0)) > 0)  
  	{
  		
      	printf("client<%u>$ %s",(unsigned int)tid,command);	
      	if (strcmp(command, "\n"))
     	{
	     	if (!strcmp(removeNewlineString(command), "exit"))
			{
				break;			
			}
	       	runShellCommand(command, 1, connfd);
	    	memset(&command[0], 0, sizeof(command));
     	}
     	else
     	{
     		write(connfd, "", 1);
     	}
    	
	}

	if (n < 0)
		printf("Command read error thread_id: %u\n", (unsigned int)tid);
	close(connfd);
	return((void *)0);
}


int main (int argc, char **argv)
{ 
	int listenfd, connfd, n;
	pid_t childpid;
	socklen_t clilen;
	char buf[MAXLINE], buf_upper[MAXLINE];
	struct sockaddr_in cliaddr, servaddr;
	pthread_t thr;

	//Create a socket for the soclet
	//If sockfd<0 there was an error in the creation of the socket
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) 
	{
		perror("Problem in creating the socket");
	    exit(2);
	}

   //preparation of the socket address
  	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

  	//bind the socket
  	bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  	//listen to the socket by creating a connection queue, then wait for clients
  	listen (listenfd, LISTENQ);
  	printf("%s\n","Server running...waiting for connections.");

  	for ( ; ; ) 
  	{
    	clilen = sizeof(cliaddr);

	    //accept a connection
	    connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
	    printf("%s\n","SERVER: Received request...");

	    int *connfd_temp = malloc(sizeof(*connfd_temp));
	    *connfd_temp = connfd ;
	    pthread_create(&thr, NULL, shell, connfd_temp);
	}
}

