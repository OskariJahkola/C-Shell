#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LOGOUT 60
#define MAXNUM 40
#define MAXLEN 160


void sighandler(int sig)
{
	/*Logout after 60 seconds of being idle*/
	switch (sig) {
		case SIGALRM:
			printf("\nautologout\n");
			exit(0);
		default:
			break;
	}
	return;
}


/* Write commands to history file */
void historyf(char* line, char historyf[]) {


	FILE *history = fopen(historyf, "a");
	fprintf(history, "%s\n", line);
	fclose(history);
	
}

/* Print entire history file */

void history_log(char historyf[]) {

	FILE *history;
	history = fopen(historyf, "r");	
	char line[256];
	int i = 0;

	
	while(fgets(line, sizeof(line), history) != NULL) {
		for (i = 0; i <= sizeof(history); i++)
		{
			if (i == sizeof(history - 1)){
				printf("%s", line);
			}
			
		}
		
	}
	
	fclose(history);
}


/* Get a specific command from history file*/

char* getCommand(int num, char historyf[]) {
	
	FILE *history = fopen(historyf, "r");	
	char line[256];
	int i = 1;
	char *cmd, * args[MAXNUM];
	
	while(fgets(line, sizeof(line), history) != NULL) {
		if (i == num) {
			printf("%s", line);
			cmd = line;
			break;
		} 
		i++;
	
	}
	fclose(history);
	return cmd;
	}


/*Change directory function*/
void change_directory(char * cd){
	
			if (cd == NULL){
			const char* home = getenv("HOME");
			chdir(home);
			}
			char *directory = cd;
			chdir(directory);
}

/*Pipe function*/
void pipe_function(char * left, char * right){
	
			char read_buffer[100];
			FILE *pipe_in;
			FILE *pipe_out;
			
			/*pipe_in is the left side of the pipe and pipe_out the right*/
			pipe_in = popen(left, "r");
			pipe_out = popen(right, "w");
			
			while(fgets(read_buffer, 100, pipe_in)){
				  fputs(read_buffer, pipe_out);
			}
			pclose(pipe_in);
			pclose(pipe_out);
			
}


int main(void)
{

	char * cmd, line[MAXLEN], promptO[MAXLEN], promptI[MAXLEN], pipes[MAXNUM], * args[MAXNUM];
	int background, i, k, j;
	int pid;
	char buff[PATH_MAX + 1];
	char* cwd;
	char* command;
	int keyr = 0;
	int keyw = 0;
	char output;
	char input;
	char historyFile[1000];


	signal(SIGALRM, sighandler);
	signal(SIGINT, sighandler);
	
	/* Specify history file's path, so that every command will be written in the 
	same file, regardless of current directory*/
	
	getcwd(historyFile, sizeof(historyFile));
	strcat(historyFile, "/history.txt");

	
	while (1) {
	
		background = 0;
		
		/*get current directory path*/
		
		cwd = getcwd(buff, PATH_MAX + 1);

		if (cwd != NULL) {
			printf("%s ", cwd);
		}


		/* set the timeout for alarm signal (autologout) */
		alarm(LOGOUT);
		
		/* read the users command */
		if (fgets(line,MAXLEN,stdin) == NULL) {
			printf("\nlogout\n");
			exit(0);
		}
		
		line[strlen(line) - 1] = '\0';
		
		if (strlen(line) == 0){
			continue;	// stop loop from continuing and start from the beggining
		}
		/* start to background */
		if (line[strlen(line)-1] == '&') {
			line[strlen(line)-1]=0;
			background = 1;
		}

		
		/*Write to history file*/
		historyf(line, historyFile);		

		/* split the command line */
		i = 0;
		cmd = line;
		
		/*Check if user is piping*/
		if((strchr(line, '|')) != NULL){
			
			/*Delete '|' symbol by parseing*/
			while ( (args[i] = strtok(cmd, "|")) != NULL) {
				i++;
				cmd = NULL;
				}
			args[0][strlen(args[0]) - 1] = '\0';
			args[1] = args[1] + 1;
			/*Call pipe_function with parameters*/
			pipe_function(args[0], args[1]);
			continue;
			/*Parse users command normally when not piping*/
		} else {

			while ( (args[i] = strtok(cmd, " ")) != NULL) {
				i++;
				cmd = NULL;
			}
		}
		
		/*Check if users first symbol is '!' and seperate the integer from it*/
		if (strlen(args[0]) >= 2 && args[0][0] == '!' && atoi(args[0]+1)) {
			int command_number = atoi(args[0]+1);
			/*Set the command to 'command' varialbe, parse it, then run it normally*/
			command = getCommand(command_number, historyFile);
			command[strlen(command) - 1] = '\0';
			while ( (args[j] = strtok(command, " ")) != NULL){
				j++;
				command = NULL;
			}
		}
		
		/*Check for redirection*/
		while (args[k] != NULL){
			if (strcmp(args[k], "<") == 0){
				keyr = 1;
				args[k] = NULL; // Remove '<' -symbol
				strcpy(promptI, args[k+1]); // Set promptI - variable to the argument after '<' -symbol
			}
			else if (strcmp(args[k], ">") == 0){
				keyw = 1;
				args[k] = NULL; // Remove '>' -symbol
				strcpy(promptO, args[k+1]); // Set promptO - variable to the argument after '>' -symbol
				}
			k++;
		}
		/*Change directory, exit and history log check*/
		
		if (strcmp(args[0], "cd")==0) {

			change_directory(args[1]);
			continue;
		}
	
		if (strcmp(args[0],"exit")==0) {
			exit(0);
		}

		if (strcmp(args[0], "history")==0) {
			history_log(historyFile);
			continue;
		}	
		
		/* fork to run the command */
		switch (pid = fork()) {
			case -1:
				/* error */
				perror("fork");
				continue;
			case 0:
			
				/* child process */

					/*Redirections*/
					if (keyw == 1){
						output = creat(promptO, 0644);
						dup2(output, STDOUT_FILENO);
						keyw = 0;
						close(output);
					}
					if (keyr == 1){
						input = open(promptI, O_RDONLY, 0);
						dup2(input, STDIN_FILENO);
						keyr = 0;
						close(input);
					}

					execvp(args[0], args);
					perror("execvp");
					exit(1);
		
			default:

				if (!background) {
					alarm(0);
					while (wait(NULL)!=pid)
						printf("some other child process exited\n");
				}
				break;
				
		}
	
	}

	return 0;
}
