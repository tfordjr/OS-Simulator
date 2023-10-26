#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "clock_shm.h"
#include "process_table_shm.h"
#include "message.h"

volatile sig_atomic_t term = 0;   // Kill signal
void sigHandler(int sig);

int main(int argc, char* argv[]){	
	sigset_t mask;          // Signal Handling
	sigdelset(&mask, SIGTERM);
	sigdelset(&mask, SIGUSR1);
	sigdelset(&mask, SIGUSR2);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	signal(SIGUSR1, sigHandler);
	signal(SIGUSR2, sigHandler);

	Clock* clock;           // init shm clock
	key_t key = ftok("/tmp", 35);
	int shmtid = shmget(key, sizeof(Clock), 0666);
	clock = shmat(shmtid, NULL, 0);

	int maxProc = 18;      // init shm process control table
	PCB* pct;
	key = ftok("/tmp", 50);
	int shmpid = shmget(key, maxProc * sizeof(PCB), 0666);
	pct = shmat(shmpid, NULL, 0);
  
	struct msg_struc message;    // create msg queue for signalling
	key = ftok("/tmp", 65);          // between oss and children
	int msgid = msgget(key, 0666);
		
	srand(getpid());      // srand random seeds
	int ran;
	int r = 0;
	int s = 0;
	//int p = 0;

	int round = 0;
	int index = atoi(argv[1]);
	int done = 0;
	int start;
	int end;

	//Critical section loop
	while ( done == 0 && term == 0 ) {
      	
		     // Check Message from oss.c
		msgrcv(msgid, &message, sizeof(message), pct[index].pid, 0);
		
		round++;
		if (round == 1)
			start = clock->secs*1000000000 + clock->nanos;    //Set the start time at round 1
			
		ran = rand()%499 + 1;
		if ( ran < 5 ) {			//Determine burst time
			pct[index].burst_time = rand()%(pct[index].burst_time - 2) + 1;
			if ( (pct[index].burst_time + pct[index].cpu_time) >= pct[index].duration) {
				pct[index].burst_time = pct[index].duration - pct[index].cpu_time;
			}

					//Update process control block
			pct[index].cpu_time += pct[index].burst_time;
			pct[index].done = 1;
			
			clock->nanos += pct[index].burst_time;    // Update clock
			while (clock->nanos > 1000000000) {
				clock->nanos -= 1000000000;
				clock->secs++;
			}

				// get runtime
			end = clock->secs*1000000000 + clock->nanos;
			int childNans = end - start;
			int childSecs = 0;
			while (childNans >= 1000000000) {
				childNans -= childNans;
				childSecs++;
			}
			pct[index].total_sec = childSecs;
			pct[index].total_nano = childNans;
					
			message.type = getppid();
			msgsnd(msgid, &message, sizeof(message), 0);
			
			shmdt(clock);   //Detach shm
			shmdt(pct);
			return -1;
      	}

			//Determine if process get blocked
		ran = rand()%99 + 1;
			//Not blocked
		if ( ran >= 20 ) {
			//Check if done
			if ( (pct[index].burst_time + pct[index].cpu_time) >= pct[index].duration) {
				pct[index].burst_time = pct[index].duration - pct[index].cpu_time;
				pct[index].done = 1;
				done = 1;
			}
 
			//Update clock
			clock->nanos += pct[index].burst_time;
			while (clock->nanos > 1000000000) {
				clock->nanos -= 1000000000;
				clock->secs++;
			}

			//Update process control block
			pct[index].cpu_time += pct[index].burst_time;

			//Send to parent
			message.type = getppid();
			msgsnd(msgid, &message, sizeof(message), 0);
      	} else {        // else if blocked
			pct[index].ready = 0;
			
			r = rand()%5;    // find out blocked time
			s = rand()%1000;
			
			pct[index].burst_time = rand()%(pct[index].burst_time - 2) + 1;
			
			if ( (pct[index].burst_time + pct[index].cpu_time) >= pct[index].duration) {
				pct[index].burst_time = pct[index].duration - pct[index].cpu_time;
				pct[index].done = 1;
				done = 1;
			}

			clock->nanos += pct[index].burst_time;    // update clock
			while (clock->nanos > 1000000000) {
				clock->nanos -= 1000000000;
				clock->secs++;
			}

			pct[index].cpu_time += pct[index].burst_time;   // update process table
			pct[index].s = r + clock->secs;
			pct[index].s = s + clock->nanos;
			while (pct[index].s >= 1000000000) {
				pct[index].s -= 1000000000;
				pct[index].r++;
			}
			
			message.type = getppid();     // Send msg   
			msgsnd(msgid, &message, sizeof(message), 0);
      	}
   	}
   
	end = clock->secs*1000000000 + clock->nanos;    // update clock with runtime
	int childNans = end - start;
	int childSecs = 0;

	while (childNans >= 1000000000) {
		childNans -= childNans;
		childSecs++;
	}

	pct[index].total_sec = childSecs;
	pct[index].total_nano = childNans;
		
	shmdt(clock);
	shmdt(pct);
  
    return 0;
}

void sigHandler(int sig){
	printf("slave: Child Pid %d signaled: %d: ...\n", getpid(), sig);
	switch(sig){
		case SIGINT:
			kill(0, SIGUSR1);
			term = 1;
			break;
		case SIGALRM:
			kill(0, SIGUSR2);
			term = 2;
			break;
	}
}
