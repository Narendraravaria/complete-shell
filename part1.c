#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

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

char *removeNewline(char *);
void pr_exit_status(int );
int main()
{
		
	//START OF SHELL TERMINAL....
	printf("$ ");
	fflush(stdout);

	char s[100];
	char **arg = malloc(64*sizeof(char*)) ; 
	char *command, *st;
	int n_size ,i, j, cmd_flag, redir_flag, pipe_flag, str_len;
	char word[100];
	while(fgets ( s, sizeof(s), stdin ) != NULL)
	{
		n_size = 0;
		st =removeNewline(s);
		str_len = strlen(st);
		command = strtok(st,"|");	// Delimit for PIPE

		while(command!=NULL)
		{
			arg[n_size++] = command; 
			command = strtok(NULL,"|");
		}// INNER WHILE END

 		if (n_size > 1) // PIPE
 		{
 			arg[n_size++]= (char *)0; //null terminated array
			i = 0;
			printf("Command: %s\n",arg[i]);
			i++;
			while(arg[i])
			{
				
				printf("PIPE\n");
				printf("Command: %s\n",arg[i]);
				i++;
			}
			pipe_flag = 1;
 		}
 		else // REDIRECTION TOKENIZATION
 		{
	 		i = 0, j = 0;
	 		n_size = 0;
	 		cmd_flag = 0;

	 		while(i < str_len)
	 		{
	 			if (st[i] == '>' || st[i] == '<')
	 			{
	 				redir_flag = 1;
	 				word[j] = '\0';
	 				if (st[i+1] == '>') //  REDIRECTIOn ">>
	 				{
	 					arg[n_size++] = word;
	 					arg[n_size++] = ">>";
	 					if (cmd_flag == 0) // FOR COMMAND
	 					{
	 						printf("Command: %s\n", arg[n_size - 2]);
	 						printf("FILE REDIRECTION: %s\n", arg[n_size - 1]);
	 						cmd_flag = 1;
	 					}
	 					else	// FOR FILE
	 					{
	 						printf("FILE: %s\n",arg[n_size - 2]);
	 						printf("FILE REDIRECTION: %s\n", arg[n_size - 1]);
	 					}
	 					i++;	// FOR DOUBLE CHECK ON ">>""
	 				}
	 				else 
	 				{
	 					arg[n_size++] = word;
	 					arg[n_size++] = (st[i] == '>') ? ">": "<";
		 				if (cmd_flag == 0)	// FOR COMMAND
	 					{
	 						printf("Command: %s\n", arg[n_size - 2]);
	 						printf("FILE REDIRECTION: %s\n", arg[n_size - 1]);
	 						cmd_flag = 1;
	 					}
	 					else 	// FOR FILE
	 					{
	 						printf("FILE: %s\n",arg[n_size - 2]);
	 						printf("FILE REDIRECTION: %s\n", arg[n_size - 1]);
	 					}

	 				} // INNER ELSE
	 				j = -1;	// RESTART WORD INDEX
	 			} // MAIN IF

	 			if (j == -1 && i != 0)
	 			{
	 				memset(word, 0, sizeof word);
	 				j++;
	 			}
	 			else
	 			{
	 				word[j] = st[i];
	 				j++;
	 			}	
	 			i++;
	 		}

	 		if (cmd_flag == 1) 	// FOR LAST WORD
	 		{
	 			arg[n_size++] = word;
	 			printf("FILE: %s\n",arg[n_size - 1]);
	 			arg[n_size++]= (char *)0; 
	 		}
	 	}

	 	// Command without PIPE and REDIRECTIOn 
	 	if (redir_flag != 1 && pipe_flag != 1)
	 	{
	 		n_size = 0;
	 		command = strtok(st," ");

			while(command!=NULL)
			{
				arg[n_size++] = command; 
				command = strtok(NULL," ");
			}// IN
			arg[n_size++]= (char *)0; 

			printf("Command: %s\n",arg[0]);
			printf("Options: %s\n",arg[1]);
			printf("Arguments: %s\n",arg[2]);		
	 	}
	 	cmd_flag = 0;
 		redir_flag = 0;
 		pipe_flag  = 0;
 				
		//FORK, EXEC & WAIT ...
		pid_t pid;
		int status;
		
		if((pid = fork()) <0)
		{
			printf("fork failed!\n");
		}else if(pid == 0)
		{
			if(execvp(arg[0],arg) < 0)
				printf("exec error\n");
		}else if(pid >0)
		{
			if(wait(&status) != pid)
				printf("wait error\n");
			pr_exit_status(status);
		}// IF END

		//READY TO RECEIVE NEXT SHELL COMMAND.....		
		printf("$ ");
		fflush(stdout);
	}//OUTER WHILE END

	// FREE ALLOCATED MEMORY...
		free (arg);	

	printf("\n");
}// MAIN END



void pr_exit_status(int status)
{
	if(WIFEXITED(status))
		printf("Normal termination, exit status = %d\n", WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		printf("Termination due to signal, signal number = %d\n",WTERMSIG(status));
	if (WCOREDUMP(status))
		printf("Core file for terminated child process is generated\n");
	if (WIFSTOPPED(status))
		printf("Child stopped, signal number = %d\n", WSTOPSIG(status));

}

char *removeNewline(char *s)
{
	int slen = strlen(s);
	if(slen>0 && s[slen-1] == '\n')
	{
		s[slen-1] = '\0';
	}
	return s;
}
