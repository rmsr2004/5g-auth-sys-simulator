// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "sm_lib.h"
#include "../log/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

pthread_t receiver_thread, sender_thread;
int receiver_id = 0, sender_id = 1;

void auth_request_manager(){
    update_log("PROCESS AUTHORIZATION_REQUEST_MANAGER CREATED");

    /*
    *   Create Receiver and Sender threads 
    */
    pthread_create(&receiver_thread, NULL, receiver, &receiver_id);
    pthread_create(&sender_thread, NULL, sender, &sender_id);


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
    
    // ...
    
    pthread_exit(NULL);
    return NULL;
}

void* sender(void* id){
    update_log("THREAD SENDER CREATED");
        
    // ...
    
    pthread_exit(NULL);
    return NULL;
}

void remove_line_break(char* string){
    for(size_t i = 0; i < strlen(string); i++){
        if(string[i] == '\n')
            string[i] = '\0';
    }
    return;
}