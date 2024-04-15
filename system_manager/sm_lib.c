// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "sm_lib.h"
#include "../log/log.h"
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/select.h>

pthread_t receiver_thread, sender_thread;
int receiver_id = 0, sender_id = 1;

void auth_request_manager(){
    update_log("PROCESS AUTHORIZATION_REQUEST_MANAGER CREATED");

    // Creates unnamed pipes
    int unnamed_pipes[AUTH_SERVERS][2];
    for(int i = 0; i < AUTH_SERVERS; i++){
        pipe(unnamed_pipes[i]);
    }
    
    // Creates user and back pipes if it doesn't exist yet
	if((mkfifo(USER_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)){
		perror("Error creating user_pipe");
		exit(1);
	}
    if((mkfifo(BACK_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)){
        perror("Error creating back_pipe");
        exit(1);
    }

    /*
    *   Create Receiver and Sender threads 
    */
    pthread_create(&receiver_thread, NULL, receiver, &receiver_id);
    pthread_create(&sender_thread, NULL, sender, &sender_id);

    // Closing unnamed pipes
    for(int i = 0; i < AUTH_SERVERS; i++)
        for(int j = 0; j < 2; j++)
            close(unnamed_pipes[i][j]);
    
    // Closing named pipes
    //close(fd_user_pipe);
    //close(fd_back_pipe);
    //unlink(USER_PIPE);
    //unlink(BACK_PIPE);

    // waits for them to die
    pthread_join(receiver_thread, NULL);
    pthread_join(sender_thread, NULL);

    exit(0);
}

void monitor_engine(){
    update_log("PROCESS MONITOR_ENGINE CREATED");
    exit(0);
} 

void* receiver(void* id){
    update_log("THREAD RECEIVER CREATED");
    
    fd_set read_set;

    // Opens pipes for reading
	if((fd_back_pipe = open(BACK_PIPE, O_RDWR)) < 0){
		perror("Error opening back_pipe");
		exit(1);
	}
    if((fd_user_pipe = open(USER_PIPE, O_RDWR)) < 0){
		perror("Error opening user_pipe");
		exit(1);
	}

    while(1){
        char* message_from_pipe = (char*) malloc(sizeof(char) * 100);
        if(message_from_pipe == NULL){
            fprintf(stderr, "Error malloc()\n");
            return NULL;
        }

        // I/O Multiplexing
		FD_ZERO(&read_set);
		FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if(select(max(fd_user_pipe, fd_back_pipe) + 1, &read_set, NULL, NULL, NULL) > 0){
            if(FD_ISSET(fd_user_pipe, &read_set)){
                // Read from user_pipe
                strcpy(message_from_pipe, read_from_user_pipe());
                printf("Message from user_pipe: %s\n", message_from_pipe);
                free(message_from_pipe);
                FD_CLR(fd_user_pipe, &read_set);
            }
            
            if(FD_ISSET(fd_back_pipe, &read_set)){
                // Read from back_pipe
                strcpy(message_from_pipe, read_from_back_pipe());
                printf("Message from back_pipe: %s\n", message_from_pipe);
                free(message_from_pipe);
                FD_CLR(fd_back_pipe, &read_set);
            }
        }
    }
    close(fd_user_pipe);
    close(fd_back_pipe);

    pthread_exit(NULL);
    return NULL;
}

void* sender(void* id){
    update_log("THREAD SENDER CREATED");
        
    // ...
    
    pthread_exit(NULL);
    return NULL;
}

char* read_from_back_pipe(){
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        fprintf(stderr, "Error malloc()\n");
        return NULL;
    }

    if(read(fd_back_pipe, message, MESSAGE_SIZE) == -1){
        fprintf(stderr, "read\n");
        return NULL;
    }
    return message;
}

char* read_from_user_pipe(){
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        fprintf(stderr, "Error malloc()\n");
        return NULL;
    }

    if(read(fd_user_pipe, message, MESSAGE_SIZE) == -1){
        perror("read");
        return NULL;
    }
    return message;
}

void handle_sigint(){
    update_log("SIGINT RECEIVED");

    /*
    *   Waiting for processes to be finished
    */
    //waitpid(auth_pid, NULL, 0);
    //waitpid(monitor_pid, NULL, 0);
}

int max(int a, int b){
    return a > b ? a : b;
}