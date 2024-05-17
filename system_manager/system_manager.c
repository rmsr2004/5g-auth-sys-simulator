// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "sm_lib.h"
#include "../log/log.h"
#include "../shared_memory/shm_lib.h"
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

/*********************************************************************************************************
*                                       SYSTEM MANAGER                                                   *                                           
**********************************************************************************************************
*   This process reads the initial settings and starts up the entire system.                             *               
*   Specifically, it has the following functions:                                                        *
*       - Reads and validates the information in the configuration file - Config File.                   *                
*       - Creates the Authorisation Requests Manager and Monitor Engine processes                        *
*       - Creates the Message Queue and Semaphore                                                        *                                                                        
*       - Creates the Shared Memory;                                                                     *
*       - Writes to the log file;                                                                        *
*       - Captures the SIGINT signal to terminate the programme, releasing all resources instead.        *
*                                                                                                        *
*   Command syntax:                                                                                      *                                                                            
*       $ 5g_auth_platform {config-file}                                                                 *
*********************************************************************************************************/

/*
*   System Manager SIGINT handler.
*   Sends SIGINT to Authorization Request Manager and waits for it to finish.
*/
void sm_sigint_handler(){
    printf("\n");
    update_log("SIGINT RECEIVED");

    kill(auth_pid, SIGINT);
    kill(monitor_pid, SIGINT);

    waitpid(auth_pid, NULL, 0);
    waitpid(monitor_pid, NULL, 0);

    close_log();
    
    exit(0);
}

int main(int argc, char* argv[]){
    signal(SIGINT, sm_sigint_handler);
    signal(SIGCHLD, SIG_IGN); // Avoid that child processes become zombies
                              // Before this implementation, that was happening a lot

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
    *   Check if Config File is valid
    */
    FILE* f = fopen(argv[1], "r");
    if(f == NULL){
        update_log("ERROR OPENING CONFIG FILE");
        close_log();
        return -1;
    }

    update_log("PROCESS SYSTEM_MANAGER CREATED");

    /*
    *   Verify config file.
    */

    char line[10];
    int count = 0;
    int config_values[6];
    while(fgets(line, 9, f) != NULL){
        remove_line_break(line);
        if(!is_number(line)){
            update_log("CONFIG FILE: LINE %d MUST BE A NUMBER!", count+1);
            close_log();
            return -1;
        }
        
        if(atoi(line) == 0){
            update_log("CONFIG FILE: LINE %d CANNOT BE 0!", count+1);
            close_log();
            return -1;
        }
        config_values[count] = atoi(line);
        count++;
    }
    fclose(f);
    
    if(count != 6){
        update_log("CONFIG FILE MUST HAVE 6 LINES!");
        close_log();
        return -1;
    }
    if(config_values[1] < 0){
        update_log("CONFIG FILE: QUEUE_POS MUST BE GREATER THAN OR EQUAL TO 0");
        close_log();
        return -1;
    }
    if(config_values[2] < 1){
        update_log("CONFIG FILE: AUTH_SERVERS MUST BE GREATER THAN OR EQUAL TO 1");
        close_log();
        return -1;
    }
    if(config_values[4] < 1){
        update_log("CONFIG FILE: MAX_VIDEO_WAIT MUST BE GREATER THAN OR EQUAL TO 1");
        close_log();
        return -1;
    }
    if(config_values[5] < 1){
        update_log("CONFIG FILE: MAX_OTHERS_WAIT MUST BE GREATER THAN OR EQUAL TO 1");
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
    * Create shared memory and his semaphore
    */

    shmid = create_shared_memory(MOBILE_USERS);
    update_log("SHARED MEMORY AND HIS SEMAPHORE CREATED");

    /*
    * Create Message Queue and his semaphore
    */

    create_message_queue();
    update_log("MESSAGE QUEUE AND HIS SEMAPHORE CREATED");

    /*
    *   Create Authorization Request Manager and Monitor Engine processes
    */

    if((auth_pid = fork()) == 0){
        // Authorization Request Manager process
        auth_request_manager();
        exit(0);
    } else if(auth_pid == -1){
        update_log("ERROR CREATING AUTHORIZATION REQUEST MANAGER");
        return -1;
    }

    if((monitor_pid = fork()) == 0){
        // Monitor Engine process
        monitor_engine();
        exit(0);
    } else if(monitor_pid == -1){
        update_log("ERROR CREATING MONITOR ENGINE");
        return -1;
    }
    
    while(1){
        pause();
    }

    return 0;
} 

// system_manager.c