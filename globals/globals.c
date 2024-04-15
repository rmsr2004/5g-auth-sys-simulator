#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int fd_user_pipe, fd_back_pipe;
pid_t auth_pid, monitor_pid;

int MOBILE_USERS,
    QUEUE_POS,
    AUTH_SERVERS,
    AUTH_PROC_TIME,
    MAX_VIDEO_WAIT,
    MAX_OTHERS_WAIT;

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