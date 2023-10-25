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
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);

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
	int p = 0;

	int round = 0;
	int index = atoi(argv[1]);
	int done = 0;
	int start;
	int end;

	//Critical section loop
	while ( done == 0 && term == 0 ) {

      	//Receive message from master
		msgrcv(msgid, &message, sizeof(message), pct[index].pid, 0);

		//Increment round
		round++;
		if (round == 1)
			start = timer->secs*1000000000 + timer->nanos;    //Set the start time at round 1

		//Small change of termination
		ran = rand()%499 + 1;
		if ( ran < 5 ) {
			//Determine burst time
			pct[index].burst_time = rand()%(pct[index].burst_time - 2) + 1;
			if ( (pct[index].burst_time + pct[index].cpu_time) >= pct[index].duration) {
			pct[index].burst_time = pct[index].duration - pct[index].cpu_time;
			}

			//Update process control block
			pct[index].cpu_time += pct[index].burst_time;
			pct[index].done = 1;

			//Update timer
			timer->nanos += pct[index].burst_time;
			while (timer->nanos > 1000000000) {
			timer->nanos -= 1000000000;
			timer->secs++;
			}

			//Get run time
			end = timer->secs*1000000000 + timer->nanos;
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
			
			shmdt(timer);   //Detach shm
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
 
			//Update timer
			timer->nanos += pct[index].burst_time;
			while (timer->nanos > 1000000000) {
				timer->nanos -= 1000000000;
				timer->secs++;
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

			timer->nanos += pct[index].burst_time;    // update clock
			while (timer->nanos > 1000000000) {
				timer->nanos -= 1000000000;
				timer->secs++;
			}

			pct[index].cpu_time += pct[index].burst_time;   // update process table
			pct[index].s = r + timer->secs;
			pct[index].s = s + timer->nanos;
			while (pct[index].s >= 1000000000) {
				pct[index].s -= 1000000000;
				pct[index].r++;
			}
			
			message.type = getppid();     // Send msg   
			msgsnd(msgid, &message, sizeof(message), 0);
      	}
   	}
   
	end = timer->secs*1000000000 + timer->nanos;    // update clock with runtime
	int childNans = end - start;
	int childSecs = 0;

	while (childNans >= 1000000000) {
		childNans -= childNans;
		childSecs++;
	}

	pct[index].total_sec = childSecs;
	pct[index].total_nano = childNans;
		
	shmdt(timer);
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