#ifndef PROCESS_TABLE_SHM_H
#define PROCESS_TABLE_SHM_H

typedef struct
{
   int pid;
   int priority;
   int cpu_time;
   int total_sec;
   int total_nano;
   int burst_time;
   int running;
   int ready;
   int duration;
   int done;
   int r;
   int s;
} PCB;

#endif
