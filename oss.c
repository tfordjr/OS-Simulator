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
#include <sys/msg.h>

#include "config.h"
#include "clock_shm.h"
#include "process_table_shm.h"

#define SEM_NAME "/file_semaphore"


void help();
void sigCatch(int);
void timeout(int);
void logfile();
void forkandwait(int);

int main(int argc, char** argv){
	int option;   
  	int numChildren = 20; 
	char logfilename[20];
        while ( (option = getopt(argc, argv, "hs:l:")) != -1) {    // Only one arg: t- 
                switch(option) {
			case 'h':
				help();
				exit(0);			
                  	case 's':                    
		    		for (int i = 1; i < argc; i++) {   //  cycles through args to find -t seconds 
       			 		if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            		 			timeoutSeconds = atoi(argv[i + 1]);  // casts str to int
            	    	 			break;
        	         		}
		    		}	
       			case 'l':
				for (int i = 1; i < argc; i++) {
					if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
						strcpy(logfilename, argv[i + 1]);
						printf("File name: %s\n", logfilename);
						break;
					}
				}				
	  	  	default:
		  		break;                        	
		}
	}

	signal(SIGINT, sigCatch);  //Signal catching setup
	signal(SIGALRM, timeout);
	alarm(timeoutSeconds);

	struct LogicalClock* clock = initClockShm();	
	struct PCB* processTable = initProcessTableShm();

	key_t messageKey = ftok(".", 'm');
	int messageQueue = msgget(messageKey, IPC_CREAT | 0666);



	while(1) {
		unsigned int nsIncrement = rand() % MAX_TIME_BETWEEN_PROCS_NS;
		incrementClock(clock, 0, nsIncrement);
	
		if(clock->seconds >= MAX_TIME_BETWEEN_PROCS_SECS){
			int slot = -1;
			for (int i = 0; i < MAX_PROCESSES; i++){
				if (processTable[i].pid == -1){
					slot = i;
					break;
				}
			}

			if (slot != -1){
			
				pid_t childPid = fork();
				if (childPid == 0){
				
			}
			
			}	



		}	
	
	
	
	}

	//logfile(logfilename[20]);
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

void logfile(char outfile[]){
        char t[9];  //  Buffer for time string
        strftime(t, sizeof(t), "%T", localtime(&(time_t){time(NULL)})); // update t with current time

        //char outfile[22];
        //snprintf(outfile, sizeof(outfile), "logfile.master.%d", getpid()); //logfile name

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

void help(){
	printf("Help message:\n");
	printf("Invoke with the form oss [-h] [-s t] [-l f]\n");
	printf("-h Describe how the project should be run and then, terminate.\n");
	printf("-s t Indicate how many maximum seconds before the system terminates\n");
	printf("-l f Specify a particular name for the log file\n");

}

