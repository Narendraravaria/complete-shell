#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>	// for open,read,write,close system calls
#include <errno.h>
#include <pthread.h>

// Code prints and '$ ' and waits in while for command and argument
// while loop gets terminated when user type EOF character(^D) 
// Inside while loop: 
//					 parse the command and argument separately
//					 fork() the new process (to execute that commad) 
//					 execvp() with parameter as command and argument list got from user 
//					 wait(&status) in parent to receive exot status of CHILD process and print it.
// Program calls malloc to allocate predetermined size of an array. 
// But unnecessary waste of memory
//		Other solution can be allocate small amount of memory using malloc (depending on avg number of argument). 
//		but inside the inner while loop check the value of n_size if it reaches value equal to size allocated using malloc
//		use realloc to increase the size of array by half or same ammount as passed in malloc
//		Other Solution is allocates a memory dynamically using realloc for each new value of command 

// Function declarations
char *removeNewline(char *);
void pr_exit_status(int *);
char *removeWhiteSpace(char *);
void setupRedirection(char **);
char ** tokenizeRedirect(char *);
void runShellCommand(char *, int, int);

// Gloabl Varibales
pid_t pid;
sigset_t set;
extern int socket_redir_flag;


void *monitor(void *arg) {
  int sig=0;
  while(1) 
  {
    sigwait(&set, &sig);
    printf("\n");
    if (pid != 0)
		{
			// printf("killing........ %ld\n", pid);
			kill(pid, SIGTERM);
		}
  }
  return 0;
}


// void runShellCommand(char *s, int SOCKET_FLAG, char *output_file)
void runShellCommand(char *s, int SOCKET_FLAG, int connfd)
{
	socket_redir_flag = 0;
		
	// //START OF SHELL TERMINAL....

	// ---------------------------------------------------------------
	// char s[100];
	char **arg = malloc(64*sizeof(char*)) ; 
	char **pipe_arg = malloc(64*sizeof(char*)) ; 
	char **red_arg = malloc(64*sizeof(char*)) ; 
	char *command, *st;
	int n_size ,i, j, redir_flag, append_flag, fdin, fdout;;


	//Signal handling code
	pthread_t thr;
	sigemptyset(&set);
	sigaddset(&set,SIGTSTP);	
	sigprocmask(SIG_BLOCK, &set, 0);
	pthread_create(&thr, 0, monitor, 0);

	// ################################################ CODE START ###########################################################

	n_size = 0;
	st =removeNewline(s);
	command = strtok(st,"|");	// Delimit for PIPE

	while(command!=NULL)
	{
		arg[n_size++] = command; 
		command = strtok(NULL,"|");
	}// INNER WHILE END
	arg[n_size++]= (char *)0;

	if (n_size == 2 && !strcmp(arg[0], "exit"))
	{
		exit(0);				
	}


	int pipe_count = (n_size - 1 - 1);
	int pipefds[pipe_count][2];
	int status[n_size - 1], status1;
	int c;
	int cmd_len;
	char *rem_whit;
	char **redirect_args;
	if (pipe_count == 0)	// if no pipe
	{
		if((pid = fork()) <0)
		{
			printf("fork failed!\n");
		}else if(pid == 0)
		{
			redirect_args = tokenizeRedirect(removeNewline(s));

			// Redirection code
			// setupRedirection(redirect_args);
			// if SOCKET_FLAG is 1 and no redirection then open output.txt file and redirect the stdout to file
			// otherwise do the same thing under " if(!strcmp(redirect_args[i], "<")) " condition 
			// don't worry if both inout and ouput redirection or single outout redirection.
			// printf("redirect_args[1] : %d\n", redirect_args[1] );
			if (SOCKET_FLAG == 1)
			{
				dup2(connfd,1);
			}

			i = 0;
			while(redirect_args[i])
			{
				if (!strcmp(redirect_args[i], "<"))	// Input redirection
				{
					
					if ((fdin = open(redirect_args[i+1], O_RDWR)) < 0)
					{
						printf("file open error (fdin) %s\n", strerror(errno));
						exit(-1);
					}
					dup2(fdin,0);
					close(fdin);

					i++;
				}
				else if (!strcmp(redirect_args[i], ">"))	// Ouput redirection
				{
					
					// check flag |----------------------------------------------------------------------|
					// open output.txt file with O_TRUNC|O_WRONLY flag to make size of file = 0
					if (SOCKET_FLAG == 1)
					{
						socket_redir_flag = 1;
					}


					if ((fdout = open(redirect_args[i+1], O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0)
						printf("file open error (fdout) %s\n", strerror(errno));
					dup2(fdout,1);
					close(fdout);
					i++;
				}
				else if (!strcmp(redirect_args[i], ">>")) // Append redirection
				{
					// check flag |----------------------------------------------------------------------|
					// open output.txt file with O_TRUNC|O_WRONLY flag to make size of file = 0
					if (SOCKET_FLAG == 1)
					{
						socket_redir_flag = 1;
					}

					if ((fdout = open(redirect_args[i+1], O_CREAT|O_APPEND|O_WRONLY, 0644)) < 0)
						printf("file open error append mode(fdout) %s\n", strerror(errno));
					dup2(fdout,1);
					close(fdout);

					i++;
				}
				i++;
			}

			n_size = 0;
			command = strtok(redirect_args[0]," ");

			while(command!=NULL)
			{
				pipe_arg[n_size++] = command; 
				command = strtok(NULL," ");
			}
			pipe_arg[n_size++]= (char *)0;

			if(execvp(pipe_arg[0],pipe_arg) < 0)
				printf("exec error\n");
		}
		wait(&status1);
	} // NO PIPE IF ENDS
	else
	{
		// Create all pipes
		i =0;
		while (i < pipe_count)
		{
			pipe(pipefds[i]);
			i++;
		}

		c = 0;

		// If multiple pipe 
		while (arg[c])
		{ 

			// Creation of child processes
			//  Command
			if((pid = fork()) <0)
			{
				printf(" command child fork failed!\n");
			}else if(pid == 0)
			{

				if (SOCKET_FLAG == 1)
				{
					dup2(connfd,1);
				}

				// TOKENIZATION except for first and last command
				if (c != 0 && c != pipe_count)
				{	
					n_size = 0;
					command = strtok(arg[c]," ");
					while(command!=NULL)
					{
						pipe_arg[n_size++] = command; 
						command = strtok(NULL," ");
					}// INNER WHILE END
					pipe_arg[n_size++]= (char *)0;

				} // END OF TOKENIZATION 

				if (c == 0)	// first command 
				{
					// deal with read redirection
					// TOKENIZATION for read redirection

					n_size = 0;
					command = strtok(arg[c],"<");
					while(command!=NULL)
					{
						red_arg[n_size++] = command; 
						command = strtok(NULL,"<");
					}
					red_arg[n_size++]= (char *)0;
					if (n_size == 3)
					{
						// check flag |----------------------------------------------------------------------|
						// redirect std out to output.txt file for SOCKET  

						// perform read redirection
						rem_whit = removeWhiteSpace(red_arg[1]);
						if ((fdin = open(rem_whit, O_RDONLY)) < 0)
						{
							printf("first command file open error (fdin) %s\n", strerror(errno));
						}
						close(0);
						dup(fdin);
						close(fdin);
					}

					// TOKENIZATION for flags in command
					n_size = 0;
					command = strtok(red_arg[0]," ");
					while(command!=NULL)
					{
						pipe_arg[n_size++] = command; 
						command = strtok(NULL," ");
					}// INNER WHILE END
					pipe_arg[n_size++]= (char *)0;

				} // END OF READ REDIRECTION

				// not first command
				if (c != 0)
				{
					dup2(pipefds[c-1][0], 0);	// read from previous command
				}

				// not last command 
				if (c != pipe_count)
				{
					dup2(pipefds[c][1], 1);		// write to next command

				}

				

				if (c == pipe_count)
				{
					// deal with write redirection

					redir_flag = 0;
					append_flag = 0;

					cmd_len = strlen(arg[c]);
					n_size = 0;
					i = 0;
					while (i < cmd_len)
					{
						if (arg[c][i] == '>')
						{
							redir_flag = 1;
								if (arg[c][i+1] == '>')
								{
									append_flag = 1;
								}
						}
						i++;
					}

					// TOKENIZATION
					command = strtok(arg[c],">");
					while(command!=NULL)
					{
						red_arg[n_size++] = command; 
						command = strtok(NULL,">");
					}// INNER WHILE END
					red_arg[n_size++]= (char *)0;

					if (redir_flag == 1)
					{
						if (append_flag == 1)
						{
							// check flag |----------------------------------------------------------------------|
							// send "" to client
							if (SOCKET_FLAG == 1)
							{
								write(connfd, "", 1);
							}

							// Perform append operation
							rem_whit = removeWhiteSpace(red_arg[1]);
							if ((fdout = open(rem_whit, O_CREAT|O_APPEND|O_WRONLY, 0644)) < 0)
							{
								printf("file open error append mode(fdout)\n");
							}	
						}
						else
						{
							// check flag |----------------------------------------------------------------------|

							if (SOCKET_FLAG == 1)
							{
								write(connfd, "", 1);
							}

							// perform redirection
							rem_whit = removeWhiteSpace(red_arg[1]);
							if ((fdout = open(rem_whit, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0)
							{
								printf("file open error (fdout)\n");
							}
						}
						
						dup2(fdout,1);
						close(fdout);
					}

					// TOKENIZATION 
					n_size = 0;
					command = strtok(red_arg[0]," ");
					while(command!=NULL)
					{
						pipe_arg[n_size++] = command; 
						command = strtok(NULL," ");
					}// INNER WHILE END
					pipe_arg[n_size++]= (char *)0;

				} // END OF WRITE REDIRECTION


				// close all pipefds
				i  = 0;
				while(i < pipe_count)
				{
					close(pipefds[i][0]);
					close(pipefds[i][1]);
					i++;
				}

				if(execvp(pipe_arg[0],pipe_arg) < 0)
					printf("%s command exec error\n", pipe_arg[0]);
				exit(-3);
			} // END OF CHILD CODE
			c++;	// Go to next command
		}// END OF WHILE LOOP

		// Close all pipe for parent 
		i  = 0;
		while(i < pipe_count)
		{
			close(pipefds[i][0]);
			close(pipefds[i][1]);
			i++;
		}

		i = 0;
		while (i <= pipe_count)
		{
			wait(status+i);
			// pr_exit_status(status + i);
			i++;
		}

	} // END OF PIPE ELSE

	// ################################################ CODE END ###########################################################

	// FREE ALLOCATED MEMORY...
	free (arg);	
	free (pipe_arg);
	free (red_arg);


	// printf("\n");
}// runShellCommand END


char *removeNewline(char *s)
{
	int slen = strlen(s);
	if(slen>0 && s[slen-1] == '\n')
	{
		s[slen-1] = '\0';
	}
	return s;
}


char *removeWhiteSpace(char *s)
{
	char *temp = malloc (sizeof(char)*64);
	char *temp1 = malloc (sizeof(char)*64);
	int slen = strlen(s);
	int i =0, j= 0, flag = 0;
	while (i != slen)
	{
		if (s[i] == ' ' && flag == 0)
		{
			// DO nothing
		}
		else
		{
			temp[j] = s[i];
			j++;
			flag = 1;
		}
		i++;
	}

	// Remove trailing spaces
	i = strlen(temp);
	while(i >= 0)
	{
		if (s[i] != ' ')
		{
			break;
		}
		i--;
	}
	strncpy(temp1, temp, i);
	free(temp);
	return temp1;
}


char ** tokenizeRedirect(char *st)
{

	int i, j , n_size, cmd_flag, str_len;	
	char *word = malloc( sizeof(char) * 64 );
	char **arg = malloc(64*sizeof(char*)) ; 

	i = 0;	j = 0;	n_size = 0;	cmd_flag = 0;
	str_len = strlen(st);

	while(i < str_len)
	{
		if (st[i] == '>' || st[i] == '<')
		{
			word[j] = '\0';
			if (st[i+1] == '>') //  FOR >> REDIRECTION
			{
				char *temp = malloc( sizeof(char) * 64 );
				temp = removeWhiteSpace(word);
				arg[n_size++] = temp;
				arg[n_size++] = ">>";
				i++;	// FOR DOUBLE CHECK ON ">>""
			}
			else 
			{
				char c = st[i];
				char *temp = malloc( sizeof(char) * 64 );
				temp = removeWhiteSpace(word);
				arg[n_size++] = temp;
				arg[n_size++] = (c == '>') ? ">": "<";

			} // INNER ELSE
			j = -1;	// RESTART WORD INDEX
		} // MAIN IF

		if (j == -1 && i != 0)
		{
			free(word);
			char *word = malloc( sizeof(char) * 64 );
			j++;
		}
		else
		{
			word[j] = st[i];
			j++;
		}	
		i++;
	}
	word[j] = '\0';
	arg[n_size++] = removeWhiteSpace(word);	
	arg[n_size++]= (char *)0; 

	return arg;
}


void pr_exit_status(int *status)
{

	if (WIFSIGNALED(status) && WTERMSIG(status) == 15)
		printf("Termination due to signal, signal number = %d\n",WTERMSIG(status));
	if (WCOREDUMP(status))
		printf("Core file for terminated child process is generated\n");
	if (WIFSTOPPED(status))
		printf("Child stopped, signal number = %d\n", WSTOPSIG(status));

}


