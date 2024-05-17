// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                          Global Variables                                                             */
/* ----------------------------------------------------------------------------------------------------------------------*/
pid_t auth_pid;
pid_t monitor_pid;
int MOBILE_USERS;
int QUEUE_POS;
int AUTH_SERVERS;
int AUTH_PROC_TIME;
int MAX_VIDEO_WAIT;
int MAX_OTHERS_WAIT;
int shmid;
sem_t* shm_sem;     
void* shared_var;
/* ----------------------------------------------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                              Functions                                                                */
/* ----------------------------------------------------------------------------------------------------------------------*/

int is_number(char* str){
    for(size_t i = 0; i < strlen(str); i++){
        if(!isdigit(str[i])){
            return 0;
        }
    }
    return 1;
}

void remove_line_break(char* string){
    for(size_t i = 0; i < strlen(string); i++){
        if(string[i] == '\n')
            string[i] = '\0';
    }
    return;
}

int max(int a, int b){
    return a > b ? a : b;
}

// globals.c