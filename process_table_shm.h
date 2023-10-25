#ifndef PROCESS_TABLE_SHM_H
#define PROCESS_TABLE_SHM_H

typedef struct
{
   pid_t pid;
   int priority;
   int cpu_time;
   int total_sec;
   int total_nano;
   int burst_time;
   int running;
   int ready;   
   int done;   
} PCB;

#endif
