// Terry Ford Jr - CS 4760 - Project 2 - 09/17/23


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "config.h"

#define SEM_NAME "/file_semaphore"

void help();
void sigCatch(int);
void timeout(int);
void logfile();
void forkandwait(int);

int main(int argc, char** argv){
	int option;   
  	int numChildren; 
   	if (argc < 3) {   //  Helps to prevent users from providing too few arguments
		perror("master: Error: Not enough arguments provided\n");
		return 0;
	}

        while ( (option = getopt(argc, argv, "t:")) != -1) {    // Only one arg: t- 
                switch(option) {				
                  	case 't':                    
		    		for (int i = 1; i < argc; i++) {   //  cycles through args to find -t seconds 
       			 		if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            		 			timeoutSeconds = atoi(argv[i + 1]);  // casts str to int
            	    	 			break;
        	         		}
		    		}			   	 
	  	  	default:
		  		for (int i = 1; i < argc; i++) {    // Cycles to find final numChildren arg
					if (strcmp(argv[i], "-t") == 0 && i + 2 < argc) {
                        			if (atoi(argv[i + 2]) > maxChildren){
							numChildren = 20;//  Does not allow more than 20 children
							printf("Too many children provided! ");
							printf("Proceeding with %d children\n", maxChildren);
						} else {
							numChildren = atoi(argv[i + 2]);    
						}					
					break;
                        		} 
		  		}
		}
	}

	signal(SIGINT, sigCatch);  //Signal catching setup
	signal(SIGALRM, timeout);
	alarm(timeoutSeconds);

	forkandwait(numChildren); // Most time is spent here creating and waiting for children

	logfile();
	return 0;	
}
            
void sigCatch(int signum) {
	printf("Ending program with interrupt signal...\n");
		
	sem_t* file_semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 1);
	sem_close(file_semaphore);
	sem_unlink(SEM_NAME); //  deallocating semaphore

	logfile();
	kill(0, SIGKILL);
}

void timeout(int signum){
	printf("Timeout has occured. Now terminating all child processes.\n ");
	
	sem_t* file_semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 1);  
	sem_close(file_semaphore);
	sem_unlink(SEM_NAME);  //  deallocating semaphore
	
	logfile();
	kill(0, SIGKILL);
}

void logfile(){
        char t[9];  //  Buffer for time string
        strftime(t, sizeof(t), "%T", localtime(&(time_t){time(NULL)})); // update t with current time

        char outfile[22];
        snprintf(outfile, sizeof(outfile), "logfile.master.%d", getpid()); //logfile name

        FILE* outputfile = fopen(outfile, "a");
	if (!outputfile){
		perror("master: Error: logfile file open operation failed!");
		return;
	}

	fprintf(outputfile, "%s Master process terminated - master(parent) pid: %d\n", t, getpid());
        fclose(outputfile);
}

void forkandwait(int numChildren){
	for (int i = 1; i < (numChildren + 1); i++) {
	
		pid_t childPid = fork();  // Create Child here

		if (childPid == 0 ) {   //Exec makes children execute slave executable
		 	if(execl("./slave","slave", (char *)NULL) == -1) { 
				perror("Child execution of slave failed ");				
			}	
			exit(0); 
				
		} else 	if (childPid == -1) {  // Error message for failed fork (child has PID -1)
            		perror("master: Error: Fork has failed!");
            		exit(0);
        	}     
    	}
	
	for (int i = 0; i < numChildren; i++) {  //Only the parent will reach here and wait for children
		wait(NULL);	// Parent Waiting for children
	}

	printf("Child processes have completed.\n");
}

