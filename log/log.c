// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/fcntl.h>
#include <semaphore.h>

char* log_file;
sem_t* sem_log;

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
	sem_log = sem_open("sem", O_CREAT|O_EXCL, 0700, 1);
    if(sem_log == SEM_FAILED){
        perror("Error creating semaphore");
        return;
    }

    update_log("5G_AUTH_PLATFORM SIMULATOR STARTING");
    return;
}

void update_log(const char* action, ...){
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

    sem_wait(sem_log);
    
    /*
    *   Write the action on file  
    */
    FILE *f = fopen(log_file, "a");
    if(f == NULL){
        perror("Error opening log file");
        sem_post(sem_log);
        return;
    }
    // Inicializar a lista de argumentos variáveis
    va_list args;
    va_start(args, action);

    // Imprimir a mensagem formatada no arquivo de log
    fprintf(f, "%02d:%02d:%02d ", hour, minutes, seconds);
    vfprintf(f, action, args);
    fprintf(f, "\n");

    // Limpar a lista de argumentos variáveis
    va_end(args);

    // Imprimir a mensagem formatada na saída padrão
    va_start(args, action);
    printf("%02d:%02d:%02d ", hour, minutes, seconds);
    vprintf(action, args);
    printf("\n");
    va_end(args);
    
    if(fclose(f)){
        perror("Error closing log file");
        sem_post(sem_log);
        return;
    }
    
    sem_post(sem_log);
    return;
}

void close_log(){
    update_log("5G_AUTH_PLATFORM SIMULATOR CLOSING");
        
    if(log_file != NULL)
        free(log_file);

    sem_close(sem_log);
    sem_unlink("sem");
    return;
}