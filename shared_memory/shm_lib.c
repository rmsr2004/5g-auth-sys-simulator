// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "shm_lib.h"
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>

pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex for shared memory. 

int create_shared_memory(int size){
    int shmid;
    key_t key;

    if((key = ftok(KEY_FILE, 'A')) == -1){
        perror("Shared Memory ftok():");
        exit(EXIT_FAILURE);
    }

    size_t total_size = sizeof(struct shm_mobile_data) * size + sizeof(struct shm_system_data);
    if((shmid = shmget(key, total_size, IPC_CREAT | 0700)) < 0){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

void* attach_shared_memory(int shmid){
    pthread_mutex_lock(&shm_mutex);
    void *shared_var;
	if((shared_var = shmat(shmid, NULL, 0)) == (void*) -1){
		perror("Shmat error!");
		exit(1);
	}
    return shared_var;
}

int detach_shared_memory(void*shared_var){
    int status = shmdt(shared_var);
    pthread_mutex_unlock(&shm_mutex);
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

void print_memory(struct shm_mobile_data* mobile_data, struct shm_system_data* system_data){
    printf("-----------------------------------------------------------------\n");
    printf("Mobile Data:\n");
    for(int i = 0; i < system_data->current_mobile_users; i++){
        printf("Mobile ID: %d\n", mobile_data[i].mobile_id);
        printf("Initial Plafond: %d\n", mobile_data[i].initial_plafond);
        printf("Current Plafond: %d\n", mobile_data[i].current_plafond);
    }

    if(system_data != NULL){
        printf("System Data:\n");
        printf("Current Mobile Users: %d\n", system_data->current_mobile_users);
        printf("Video Plafond Used: %d\n", system_data->video_plafond_used);
        printf("Social Plafond Used: %d\n", system_data->social_plafond_used);
        printf("Music Plafond Used: %d\n", system_data->music_plafond_used);
        printf("Video Requests: %d\n", system_data->video_requests);
        printf("Social Requests: %d\n", system_data->social_requests);
        printf("Music Requests: %d\n", system_data->music_requests);
    }
    printf("-----------------------------------------------------------------\n");
}