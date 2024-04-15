// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

#include "../log/log.h"
#include "../shared_memory/shm_lib.h"
#include "sm_lib.h"
#include "../globals/globals.h"

int shmid;
int* shared_var;

int main(int argc, char* argv[]){
    /*
    *   Create log file.
    */
    create_log_file("log/log.txt");

    /*
    *   Checks if System Manager has been run correctly  
    */
    if(argc != 2){
        fprintf(stderr, "Invalide Run\nUsage: %s {config-file}\n", argv[0]);
        close_log();
        return -1;
    }

    /*
    * Check if Config File is valid
    */
    FILE* f = fopen(argv[1], "r");
    if(f == NULL){
        perror("Error opening config_file");
        close_log();
        return -1;
    }

    update_log("PROCESS SYSTEM_MANAGER CREATED");

    char line[10];
    int count = 0;
    int config_values[6];
    while(fgets(line, 9, f) != NULL){
        remove_line_break(line);
        if(!is_number(line)){
            fprintf(stderr, "Line %d must be a number!\n", count+1);
            close_log();
            return -1;
        }
        
        if(atoi(line) == 0){
            fprintf(stderr, "Line %d cannot be 0!\n", count+1);
            close_log();
            return -1;
        }
        config_values[count] = atoi(line);
        count++;
    }
    fclose(f);
    
    if(count != 6){
        fprintf(stderr, "Config File must have 6 lines\n");
        close_log();
        return -1;
    }
    if(config_values[1] < 0){
        fprintf(stderr, "QUEUE_POS must be greater than or equal to 0\n");
        close_log();
        return -1;
    }
    if(config_values[2] < 1){
        fprintf(stderr, "AUTH_SERVERS must be greater than or equal to 1\n");
        close_log();
        return -1;
    }
    if(config_values[4] < 1){
        fprintf(stderr, "MAX_VIDEO_WAIT must be greater than or equal to 1\n");
        close_log();
        return -1;
    }
    if(config_values[5] < 1){
        fprintf(stderr, "MAX_OTHERS_WAIT must be greater than or equal to 1\n");
        close_log();
        return -1;
    }

    /*
    *   Assign values from config file to variables.
    */
    MOBILE_USERS    = config_values[0];
    QUEUE_POS       = config_values[1];
    AUTH_SERVERS    = config_values[2];
    AUTH_PROC_TIME  = config_values[3];
    MAX_VIDEO_WAIT  = config_values[4];
    MAX_OTHERS_WAIT = config_values[5];

    /*
    * Create shared memory
    */
    shmid = create_shared_memory();
    shared_var = attach_shared_memory(shmid);

    /*
    *   Create Authorization Request Manager and Monitor Engine processes
    */
    if((auth_pid = fork()) == 0){
        // Authorization Request Manager process
        auth_request_manager();
        exit(0);
    } else if(auth_pid == -1){
        perror("fork()");
        return -1;
    }

    if((monitor_pid = fork()) == 0){
        // Monitor Engine process
        monitor_engine();
        exit(0);
    } else if(monitor_pid == -1){
        perror("fork()");
        return -1;
    }

    signal(SIGINT, handle_sigint);

    waitpid(auth_pid, NULL, 0);
    waitpid(monitor_pid, NULL, 0);
    
    /*
    *   Remove Shared Memory
    */
    detach_shared_memory(shared_var);
    remove_shared_memory(shmid);
    
    close_log();
    return 0;
}