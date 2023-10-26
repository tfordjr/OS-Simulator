// Terry Ford Jr - CS 4760 - Project 4 - 10/24/23

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>

#include "clock_shm.h"
#include "process_table_shm.h"
#include "message.h"

// GLOBALS
volatile sig_atomic_t term = 0;
const int maxProc = 18;

long roundRobin[18];  // Arrays for process queues
long feedbck_1[18];
long feedbck_2[18];
long feedbck_3[18];

int quantum[4] = {2000000, 4000000, 8000000, 16000000};  // Array to hold burst times
int i;     // globals for for loops
int j;
int k;

// function headers
void help();    
void handle_signal(int sig);

int main(int argc, char* argv[]) {   	

   sigset_t mask;         // Set up signal handling
   sigfillset(&mask);
   sigdelset(&mask, SIGINT);
   sigdelset(&mask, SIGALRM);
   sigdelset(&mask, SIGTERM);
   sigprocmask(SIG_SETMASK, &mask, NULL);
   signal(SIGINT, handle_signal);
   signal(SIGALRM, handle_signal);
   
   Clock* clock;     // init shm clock
   key_t key = ftok("/tmp", 35);
   int shmtid = shmget(key, sizeof(Clock), IPC_CREAT | 0666);
   clock = shmat(shmtid, NULL, 0);
   clock->secs = 0;
   clock->nanos = 0; 
   
   PCB* pct;       // init shm process table
   key = ftok("/tmp", 50);
   int shmpid = shmget(key, maxProc * sizeof(PCB), IPC_CREAT | 0666);
   pct = shmat(shmpid, NULL, 0);
   for (i = 0; i < maxProc; i++)
      pct[i].ready = -1;
   
   for ( i = 0; i < maxProc; i++ ) {    // init process queues
      roundRobin[i] = 0;
      feedbck_1[i] = 0;
      feedbck_2[i] = 0;
      feedbck_3[i] = 0;
   }
    
   struct msg_struc message;    // init shm message queue
   key = ftok("/tmp", 65);
   int msgid = msgget(key, IPC_CREAT | 0666);

   int c = 0;              // Variables for getopt
   extern char *optarg;
   extern int optind, optopt, opterr;
   
   char* filename = "default.log";
   int termTime = 3;
   
   while ((c = getopt(argc, argv, "hl:t:")) != -1) {   // Command line args
      switch(c) {         
         case 'h':  // help
            help();
            break;

         case 'l':  // set filename            
			filename = optarg;
			printf("%s: filename set to: %s\n", argv[0], filename);       
            break;
        
         case 't':  // set time             
			termTime = atoi(optarg);
			printf("%s: termination time set to: %d\n", argv[0], termTime);                          
            break;
      }
   }

   alarm(termTime);     //Start timeout timer
   
   srand(getpid());     //Seed random number generator
   unsigned int ran;
   unsigned int secs;
   unsigned int nanos;

   //Local variables
   int total = 0;     //Total number of spawned processes
   int count = 0;     //Current number of spawned processes
   int index = 0;     //Index of process control table
   int pid;           //PID variable
   int status;        //Used for status info from child
   FILE* log;	      //File variable

   //Constant time constraints
   const int maxTimeBetweenNewProcsNS = 1000;
   const int maxTimeBetweenNewProcsSecs = 1;

   printf("%s: Computing... log filename: %s, termination time: %d\n", argv[0], filename, termTime);
   log = fopen(filename, "w");

   // MAIN LOOP
   while ( term == 0 || count > 0) {

      //Generate random time to spawn process
      secs = rand() % maxTimeBetweenNewProcsSecs;
      nanos = rand() % maxTimeBetweenNewProcsNS;
      
      //Adjust as needed
      clock->secs += secs;
      clock->nanos += nanos; 
      while (clock->nanos > 1000000000) {
         clock->nanos -= 1000000000;
         clock->secs++;
      }

      //If resources allow another process
      if ( count < 18 && total < 100 ) {

         //Find open process control block
         for (i = 0; i < maxProc; i++) {
            if (pct[i].ready == -1) {
               index = i;
               break;
            }
         }
                
         //Fork and increment total/count
         pid = fork();
         total++;
         count++;

         if (total == 100)
            term = 3;
         
         switch(pid) {    //Switch PID            
            case -1:    //Fork error
               fprintf(stderr, "%s: Error: Failed to fork slave process\n", argv[0]);
               count--;
               break;
            
            case 0:   // Child 
               //Enter of a critical section
               msgrcv(msgid, &message, sizeof(message), 1, 0);
               printf("%s: Attempting to exec child process %d\n", argv[0], getpid());
               //Exit critical section

               message.type = 2;
               msgsnd(msgid, &message, sizeof(message), 0);

               //Execute the user program
               char ind[3];
               sprintf(ind, "%d", index);
               char* args[] = {"./slave", ind, NULL};
               if (execv(args[0], args) == -1)
                  fprintf(stderr, "%s: Error: Failed to exec child process %d\n", argv[0], getpid());
               
               exit(1);            
            default: //Parent process

               pct[index].pid = pid;  // Init process table               
               ran = rand()%99 + 1;   
               
               if ( ran < 3 ) {    //Real-time:
                  pct[index].priority = 0;
                  for ( i = 0; i < maxProc; i++) {
                     if (roundRobin[i] == 0) {
                        roundRobin[i] = pct[index].pid;
                        pct[index].burst_time = quantum[0];
                        break;
                     }
                  }
               } else {     //Normal:
                  pct[index].priority = 1;
                  for ( i = 0; i < maxProc; i++) {
                     if (feedbck_1[i] == 0) {
                        feedbck_1[i] = pct[index].pid;
                        pct[index].burst_time = quantum[1];
                        break;
                     }
                  }
               }   
 
               //Generate random duration
               pct[index].duration = rand()%99999998 + 1;

               //Initialize time members
               pct[index].cpu_time = 0;
               pct[index].total_sec = 0;
               pct[index].total_nano = 0;

               //Initialize the running and ready states
               pct[index].running = 0;
               pct[index].ready = 1;
               pct[index].done = 0;

               //Initialize block variables
               pct[index].r = 0;
               pct[index].s = 0;

               //Write to log
               snprintf(message.str, sizeof(message), "OSS: Generating process with PID %d and putting it in queue %d at time %d.%d\n",
                            pct[index].pid, pct[index].priority, clock->secs, clock->nanos);
               fprintf(log, "%s", message.str);

               //Confirm the child process execution
               message.type = 1;
               msgsnd(msgid, &message, sizeof(message), 0);
               msgrcv(msgid, &message, sizeof(message), 2, 0);
               break;
         }          
      }     
         
      //Increment logical clock for scheduling work
      clock->nanos += rand()%9900 + 100;
      while (clock->nanos > 1000000000) {
         clock->nanos -= 1000000000;
         clock->secs++;
      }
 




      //Schedule round robin queue
      if (roundRobin[0] != 0) {			//Get the index number
         for (i = 0; i < maxProc; i++) {
            if (pct[i].pid == roundRobin[0]) {
               break;
            }
         }         
         pct[i].burst_time = quantum[0];  // Set time burst

         //Write to log
         snprintf(message.str, sizeof(message), "OSS: Dispaching process with PID %d from queue %d at time %d.%d\n",
         			pct[i].pid, pct[i].priority, clock->secs, clock->nanos);
         fprintf(log, "%s", message.str);
         
         message.type = roundRobin[0];
         msgsnd(msgid, &message, sizeof(message), 0);         
         msgrcv(msgid, &message, sizeof(message), (long)getpid(), 0);
        
         if (pct[i].done == 1) {            // IF DONE
            count--;
            printf("%s: Process %d finished. %d child processes running.\n", argv[0], pct[i].pid, count);
            waitpid(pct[i].pid, &status, 0);
            snprintf(message.str, sizeof(message), "OSS: Process with PID %d has finished at time %d.%d after running %d nanoseconds\n",
                                                                    pct[i].pid, clock->secs, clock->nanos, pct[i].cpu_time);
            fprintf(log, "%s", message.str);
            roundRobin[0] = 0;
            pct[i].ready = -1;
         } else if ( pct[i].ready = 0 ) {   // ELSE IF QUEUE BLOCKED
            snprintf(message.str, sizeof(message.str), "OSS: Process with PID %d blocked at time, ran for %d nanoseconds\n",
                 	  pct[i].pid, clock->secs, clock->nanos, pct[index].burst_time);
            fprintf(log, "%s", message.str);
         } else {                        // ELSE STILL RUNNING
            snprintf(message.str, sizeof(message), "OSS: Recieving that process with PID %d ran for %d nanoseconds\n",
                      pct[i].pid, pct[i].burst_time );
            fprintf(log, "%s", message.str);
         }

         //Update the priority queues
         long temp = roundRobin[0];
         for ( i = 0; i < (maxProc-1); i++) {
            roundRobin[i] = roundRobin[i+1];
         }
         roundRobin[maxProc-1] = 0;
         for ( i = 0; i < maxProc; i++) {
            if (roundRobin[i] == 0) {
               roundRobin[i] = temp;
               break;
            }
         }  
      }





      //Schedule high-priority queue
      else if (feedbck_1[0] != 0) {		//Get the index number
         for (i = 0; i < maxProc; i++) {
            if (pct[i].pid == feedbck_1[0]) {
               break;
            }
         }
         
         pct[i].burst_time = quantum[1];   //Set the burst time
 
             // log 
         snprintf(message.str, sizeof(message), "OSS: Dispaching process with PID %d from queue %d at time %d.%d\n",
                    pct[i].pid, pct[i].priority, clock->secs, clock->nanos);
         fprintf(log, "%s", message.str);
         
         message.type = feedbck_1[0];   // Allow child to run
         msgsnd(msgid, &message, sizeof(message), 0);
         msgrcv(msgid, &message, sizeof(message), (long)getpid(), 0);
               
         if (pct[i].done == 1) {                // IF DONE
            count--;
            printf("%s: Process %d finished. %d child processes running.\n", argv[0], pct[i].pid, count);
            waitpid(pct[i].pid, &status, 0);
            snprintf(message.str, sizeof(message), "OSS: Process with PID %d has finished at time %d.%d after running %d nanoseconds\n",
                        pct[i].pid, clock->secs, clock->nanos, pct[i].cpu_time);
            fprintf(log, "%s", message.str);
            feedbck_1[0] = 0;
            pct[i].ready = -1;
         } else if ( pct[i].ready = 0 ) {      // ELSE IF BLOCKED
            pct[i].priority++;
            snprintf(message.str, sizeof(message.str), "OSS: Process with PID %d blocked at time, ran for %d nanoseconds\n",
                        pct[i].pid, clock->secs, clock->nanos, pct[index].burst_time);
            fprintf(log, "%s", message.str);
         } else {                             // ELSE STILL RUNNING
            pct[i].priority++;
            snprintf(message.str, sizeof(message), "OSS: Recieving that process with PID %d ran for %d nanoseconds\n",
                        pct[i].pid, pct[i].burst_time );
            fprintf(log, "%s", message.str);

            snprintf(message.str, sizeof(message), "OSS: Putting process with PID %d into queue %d\n",
                        pct[index].pid, pct[index].priority);
            fprintf(log, "%s", message.str);
         }

         //Update the priority queues
         long temp = feedbck_1[0];
         for ( i = 0; i < (maxProc-1); i++) {
            feedbck_1[i] = feedbck_1[i+1];
         }
         feedbck_1[maxProc-1] = 0;
         for ( i = 0; i < maxProc; i++) {
            if (feedbck_2[i] == 0) {
               feedbck_2[i] = temp;
               break;
            }
         }
      }





      //Schedule medium-priority queue
      else if (feedbck_2[0] != 0) {
         for (i = 0; i < maxProc; i++) {	//Get the index number
            if (pct[i].pid == feedbck_2[0]) {
               break;
            }
         }
     
        
         pct[i].burst_time = quantum[2];   //Set the time burst
         snprintf(message.str, sizeof(message), "OSS: Dispaching process with PID %d from queue %d at time %d.%d\n",
                    pct[i].pid, pct[i].priority, clock->secs, clock->nanos);
         fprintf(log, "%s", message.str);
         
         message.type = feedbck_2[0];
         msgsnd(msgid, &message, sizeof(message), 0);        
         msgrcv(msgid, &message, sizeof(message), (long)getpid(), 0);
         
         if (pct[i].done == 1) {           // IF DONE
            count--;
            printf("%s: Process %d finished. %d child processes running.\n", argv[0], pct[i].pid, count);
            waitpid(pct[i].pid, &status, 0);
            snprintf(message.str, sizeof(message), "OSS: Process with PID %d has finished at time %d.%d after running %d nanoseconds\n",
                        pct[i].pid, clock->secs, clock->nanos, pct[i].cpu_time);
            fprintf(log, "%s", message.str);
            feedbck_2[0] = 0;
            pct[i].ready = -1;
         } else if ( pct[i].ready = 0 ) {     // ELSE IF BLOCKED
            pct[i].priority++;
            snprintf(message.str, sizeof(message.str), "OSS: Process with PID %d blocked at time, ran for %d nanoseconds\n",
                        pct[i].pid, clock->secs, clock->nanos, pct[index].burst_time);
            fprintf(log, "%s", message.str);
         } else {                             // ELSE STILL RUNNING
            pct[i].priority++;
            snprintf(message.str, sizeof(message), "OSS: Recieving that process with PID %d ran for %d nanoseconds\n",
                        pct[i].pid, pct[i].burst_time );
            fprintf(log, "%s", message.str);

            snprintf(message.str, sizeof(message), "OSS: Putting process with PID %d into queue %d\n",
                        pct[index].pid, pct[index].priority);
            fprintf(log, "%s", message.str);
         }

         //Update the priority queues
         long temp = feedbck_2[0];
         for ( i = 0; i < (maxProc-1); i++) {
            feedbck_2[i] = feedbck_2[i+1];
         }
         feedbck_2[maxProc-1] = 0;
         for ( i = 0; i < maxProc; i++) {
            if (feedbck_3[i] == 0) {
               feedbck_3[i] = temp;
               break;
            }
         }
      }





      //Schedule low-priority queue
      else if (feedbck_3[0] != 0) {
         for (i = 0; i < maxProc; i++) {	//Get the index number
            if (pct[i].pid == feedbck_3[0]) {
               break;
            }
         }
          
         pct[i].burst_time = quantum[3];     //Set the burst time

         //Write to log
         snprintf(message.str, sizeof(message), "OSS: Dispaching process with PID %d from queue %d at time %d.%d\n",
                    pct[i].pid, pct[i].priority, clock->secs, clock->nanos);
         fprintf(log, "%s", message.str);
         
         message.type = feedbck_3[0];       //Let the child process run
         msgsnd(msgid, &message, sizeof(message), 0);         
         msgrcv(msgid, &message, sizeof(message), (long)getpid(), 0);
          
         if (pct[i].done == 1) {              // IF DONE
            count--;
            waitpid(pct[i].pid, &status, 0);
            printf("%s: Process %d finished. %d child processes running.\n", argv[0], pct[i].pid, count);
            snprintf(message.str, sizeof(message), "OSS: Process with PID %d has finished at time %d.%d after running %d nanoseconds\n",
                        pct[i].pid, clock->secs, clock->nanos, pct[i].cpu_time);
            fprintf(log, "%s", message.str);
            feedbck_3[0] = 0;
            pct[i].ready = -1;
         } else if ( pct[i].ready = 0 ) {     // ELSE IF BLOCKED
            snprintf(message.str, sizeof(message.str), "OSS: Process with PID %d blocked at time, ran for %d nanoseconds\n",
                        pct[i].pid, clock->secs, clock->nanos, pct[index].burst_time);
            fprintf(log, "%s", message.str);
         } else {                            // ELSE STILL RUNNING
            snprintf(message.str, sizeof(message), "OSS: Recieving that process with PID %d ran for %d nanoseconds\n",
                        pct[i].pid, pct[i].burst_time );
            fprintf(log, "%s", message.str);
         }





         //Update the priority queues
         long temp = feedbck_3[0];
         for ( i = 0; i < (maxProc-1); i++) {
            feedbck_3[i] = feedbck_3[i+1];
         }
         feedbck_3[maxProc-1] = 0;
         for ( i = 0; i < maxProc; i++) {
            if (feedbck_3[i] == 0) {
               feedbck_3[i] = temp;
               break;
            }
         }
      }
   }
   // END OF WHILE LOOP

   //Wait for all child processes to finish
   while ( (pid = wait(&status)) > 0) {
      count--;
      printf("%s: Process %d finished. %d child processes running.\n", argv[0], pid, count);
   }

   //Report cause of termination
   if (term == 1)
      printf("%s: Ended because of user interrupt\n", argv[0]);
   else if (term == 2)
      printf("%s: Ended because of timeout\n", argv[0]);
   else if (total == 100) 
      printf("%s: Ended because total = 100\n", argv[0]);
   else
      printf("%s: Termination cause unknown\n", argv[0]);

   //Print info
   printf("%s: Program ran for %d.%d and generated %d user processes\n", argv[0], clock->secs, clock->nanos, total);

		//Free allocated memory
   shmdt(clock);
   shmctl(shmtid, IPC_RMID, NULL);
   shmdt(pct);
   shmctl(shmpid, IPC_RMID, NULL);
   msgctl(msgid, IPC_RMID, NULL);

   
   fclose(log);  // close file
   return 0;
}


void help(){
	printf("Help message:\n");
	printf("Invoke with the form oss [-h] [-s t] [-l f]\n");
	printf("-h Describe how the project should be run and then, terminate.\n");
	printf("-s t Indicate how many maximum seconds before the system terminates\n");
	printf("-l f Specify a particular name for the log file\n");

}

void handle_signal(int sig) {
   printf("./oss: Parent process %d caught signal: %d. Cleaning up and terminating.\n", getpid(), sig);
   switch(sig) {
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