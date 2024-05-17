// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "shm_lib.h"
#include "../globals/globals.h"
#include "../log/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <semaphore.h>

int create_shared_memory(int size){
    int shmid;
    key_t key;

    if((key = ftok(KEY_FILE, 'A')) == -1){
        update_log("ERROR CREATING KEY TO SHARED MEMORY");
        exit(EXIT_FAILURE);
    }

    size_t total_size = sizeof(struct shm_system_data) + sizeof(int) * (AUTH_SERVERS + 1) + sizeof(struct shm_mobile_data) * size;
    if((shmid = shmget(key, total_size, IPC_CREAT | 0600)) < 0){
        update_log("ERROR CREATING SHARED MEMORY");
        exit(EXIT_FAILURE);
    }

    // Create semaphore.
    sem_unlink(SHM_SEM);
    shm_sem = sem_open(SHM_SEM, O_CREAT | O_EXCL, 0600, 1);
    if(shm_sem == SEM_FAILED){
        update_log("ERROR CREATING SEMAPHORE");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

void* attach_shared_memory(int shmid){
    void* shared_var;
	if((shared_var = shmat(shmid, NULL, 0)) == (void*) -1){
        update_log("ERROR ATTACHING SHARED MEMORY");
		exit(1);
	}
    return shared_var;
}

int detach_shared_memory(void* shared_var){
    int status = shmdt(shared_var);
    if(status == -1){
        update_log("ERROR DETACHING SHARED MEMORY");
        exit(EXIT_FAILURE);
    }
    return status;
}

void remove_shared_memory(int shmid){
    sem_close(shm_sem);
    sem_unlink(SHM_SEM);

    detach_shared_memory(shared_var);

    if(shmctl(shmid, IPC_RMID, NULL) == -1){
        update_log("ERROR REMOVING SHARED MEMORY");
        exit(EXIT_FAILURE);
    }
}

void print_memory(struct shm_mobile_data* mobile_data, struct shm_system_data* system_data){
    printf("-----------------------------------------------------------------\n");
    if(mobile_data != NULL){
        printf("Mobile Data:\n");
        for(int i = 0; i < system_data->current_mobile_users; i++){
            printf("Mobile ID: %d\n", mobile_data[i].mobile_id);
            printf("Initial Plafond: %d\n", mobile_data[i].initial_plafond);
            printf("Current Plafond: %d\n", mobile_data[i].current_plafond);
        }
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