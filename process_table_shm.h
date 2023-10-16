#ifndef CLOCK_SHM_H
#define CLOCK_SHM_H

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CLOCK_SHM_NAME "/logical_clock"
#define CLOCK_SHM_SIZE sizeof(struct LogicalClock) 

struct LogicalClock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

struct LogicalClock* initClockShm(){
        // Allocates shared memory boolean lock. lock will ensure one child works at a time on cstest
        int shmid = shm_open(CLOCK_SHM_NAME, O_CREAT | O_RDWR, 0666);   //  Creating shared memory
        if (shmid == -1) {
                perror("initClockShm: Error: shmget failed\n");
                exit(1);
        }

        int trunc = ftruncate(shmid, CLOCK_SHM_SIZE);
        if (trunc == -1){
                perror("initClockShm: Error: truncation failed\n");
                exit(1);
        }

        struct LogicalClock* clock = (struct LogicalClock*)mmap(NULL, CLOCK_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
        if (clock == MAP_FAILED) {
                perror("initClockShm: Error: mmat failed\n");
                exit(1);
        }

        clock->seconds = 0;       // Initializing clock 
	clock->nanoseconds = 0;
     	return clock;   
}

struct LogicalClock* openClockShm(){	
	int shmid = shm_open(CLOCK_SHM_NAME, O_RDWR, 0);    //Creating shared memory
        if (shmid == -1) {
                perror("openClockShm: Error: shm_open failed\n");
                exit(1);
        }

        struct LogicalClock* clock = (struct LogicalClock*)mmap(NULL, CLOCK_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0); 
        if (clock == MAP_FAILED) {
                perror("openClockShm: Error: mmap failed\n");
                exit(1);
        }
	return clock;
}

void deallocateClockShm(){
	if(shm_unlink(CLOCK_SHM_NAME) == -1){
		perror("deallocateClockShm: Error: shm_unlink falied\n");
		exit(1);
	}
}

#endif
