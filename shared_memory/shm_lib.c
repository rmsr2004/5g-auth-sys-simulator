// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "shm_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

int create_shared_memory(){
    int shmid;
    if((shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0700)) < 0){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

int* attach_shared_memory(int shmid){
    int* shared_var;
	if((shared_var = (int *) shmat(shmid, NULL, 0)) == (int *) -1){
		perror("Shmat error!");
		exit(1);
	}
    return shared_var;
}

int detach_shared_memory(int* shared_var){
    int status = shmdt(shared_var);
    if(status == -1){
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    return status;
}

void remove_shared_memory(int shmid){
    if(shmctl(shmid, IPC_RMID, NULL) == -1){
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
}