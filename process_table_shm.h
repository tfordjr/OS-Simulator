#ifndef PROCESS_TABLE_SHM_H
#define PROCESS_TABLE_SHM_H

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "config.h"

#define PROCESS_TABLE_SHM_NAME "/process_table"
#define PROCESS_TABLE_SHM_SIZE (sizeof(struct PCB) * MAX_PROCESSES) 

struct PCB {
	pid_t pid;
	unsigned int total_cpu_time;
	unsigned int total_system_time;
	unsigned int last_burst_time;
	int priority;
};

struct PCB* initProcessTableShm(){
        // Allocates shared memory boolean lock. lock will ensure one child works at a time on cstest
        int shmid = shm_open(PROCESS_TABLE_SHM_NAME, O_CREAT | O_RDWR, 0666);   //  Creating shared memory
        if (shmid == -1) {
                perror("initProcessTableShm: Error: shmget failed\n");
                exit(1);
        }

        int trunc = ftruncate(shmid, PROCESS_TABLE_SHM_SIZE);
        if (trunc == -1){
                perror("initProcessTableShm: Error: truncation failed\n");
                exit(1);
        }

        struct PCB* processTable = (struct PCB*)mmap(NULL, PROCESS_TABLE_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
        if (processTable == MAP_FAILED) {
                perror("initProcessTableShm: Error: mmat failed\n");
                exit(1);
        }

	for (int i = 0; i < MAX_PROCESSES; i++) {
		processTable[i].pid = -1;
	}

	return processTable;   
}

struct PCB* openProcessTableShm(){	
	int shmid = shm_open(PROCESS_TABLE_SHM_NAME, O_RDWR, 0);    //Creating shared memory
        if (shmid == -1) {
                perror("openProcessTableShm: Error: shm_open failed\n");
                exit(1);
        }

        struct PCB* processTable = (struct PCB*)mmap(NULL, PROCESS_TABLE_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0); 
        if (processTable == MAP_FAILED) {
                perror("openProcessTableShm: Error: mmap failed\n");
                exit(1);
        }
	return processTable;
}

void deallocateProcessTableShm(){
	if(shm_unlink(PROCESS_TABLE_SHM_NAME) == -1){
		perror("deallocateProcessTableShm: Error: shm_unlink falied\n");
		exit(1);
	}
}

#endif
