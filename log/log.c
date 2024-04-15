// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/fcntl.h>
#include <semaphore.h>

#include "log.h"

// Global variable to save log_file once he is created.
char* log_file;

// Semaphore to avoid two process writing at same time.
sem_t* sem_bck;

void create_log_file(char* file){
    FILE *f = fopen(file, "w");  
    if(f == NULL){
        perror("Error creating log file");
        return;
    }
    
    if(fclose(f)){
        perror("Error closing log file");
        return;
    }

    /*
    * Save log_file to use in other functions
    */
    log_file = (char*) malloc(strlen(file) * sizeof(char));
    strcpy(log_file, file);

    sem_unlink("sem");
	sem_bck = sem_open("sem", O_CREAT|O_EXCL, 0700, 1);
    if(sem_bck == SEM_FAILED){
        perror("Error creating semaphore");
        return;
    }

    update_log("5G_AUTH_PLATFORM SIMULATOR STARTING");
    return;
}

void update_log(char* action){
    /*
    *   Get current time.
    */
    time_t current_time;
    struct tm *local_time;

    time(&current_time);
    local_time = localtime(&current_time);

    int hour = local_time->tm_hour;
    int minutes = local_time->tm_min;
    int seconds = local_time->tm_sec;

    sem_wait(sem_bck);
    
    /*
    *   Write the action on file  
    */
    FILE *f = fopen(log_file, "a");
    if(f == NULL){
        perror("Error opening log file");
        sem_post(sem_bck);
        return;
    }

    fprintf(f, "%02d:%02d:%02d %s\n", hour, minutes, seconds, action);
    printf("%02d:%02d:%02d %s\n", hour, minutes, seconds, action);
    
    if(fclose(f)){
        perror("Error closing log file");
        sem_post(sem_bck);
        return;
    }
    
    sem_post(sem_bck);
    return;
}

void close_log(){
    update_log("5G_AUTH_PLATFORM SIMULATOR CLOSING");
        
    if(log_file != NULL)
        free(log_file);

    sem_close(sem_bck);
    sem_unlink("sem");
    return;
}