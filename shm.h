#ifndef SHM_H
#define SHM_H

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_NAME "/logical_clock"
#define SHM_SIZE sizeof(struct LogicalClock) 

struct LogicalClock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

struct LogicalClock* initshm(){
        // Allocates shared memory boolean lock. lock will ensure one child works at a time on cstest
        int shmid = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);   //  Creating shared memory
        if (shmid == -1) {
                perror("initshm: Error: shmget failed\n");
                exit(1);
        }

        int trunc = ftruncate(shmid, SHM_SIZE);
        if (trunc == -1){
                perror("initshm: Error: truncation failed\n");
                exit(1);
        }

        struct LogicalClock* clock = (struct LogicalClock*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
        if (clock == MAP_FAILED) {
                perror("initshm: Error: mmat failed\n");
                exit(1);
        }

        clock->seconds = 0;       // Initializing clock 
	clock->nanoseconds = 0;
     	return clock;   
}

struct LogicalClock* openshm(){	
	int shmid = shm_open(SHM_NAME, O_RDWR, 0);    //Creating shared memory
        if (shmid == -1) {
                perror("openshm: Error: shm_open failed\n");
                exit(1);
        }

        struct LogicalClock* clock = (struct LogicalClock*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0); 
        if (clock == MAP_FAILED) {
                perror("openshm: Error: mmap failed\n");
                exit(1);
        }
	return clock;
}

void deallocateshm(){
	if(shm_unlink(SHM_NAME) == -1){
		perror("deallocateshm: Error: shm_unlink falied\n");
		exit(1);
	}
}

#endif
