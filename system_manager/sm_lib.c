// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "sm_lib.h"
#include "../log/log.h"
#include "../globals/globals.h"
#include "../queue/queue.h"
#include "../shared_memory/shm_lib.h"
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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/time.h>

/* ----------------------------------------------------------------------------------------------------------------------*/                         
/*                                          Global Variables                                                             */
/* ----------------------------------------------------------------------------------------------------------------------*/
pthread_t receiver_thread;                                              // Receiver thread.
pthread_t sender_thread;                                                // Sender thread.
queue_t* video_queue;                                                   // Video_Streaming_Queue.
queue_t* others_queue;                                                  // Others_Services_Queue.
pthread_cond_t* engine_available_cond;                                  // Condition variable for engine available.
pthread_mutex_t* engine_available_mutex;                                // Mutex for engine available.
int* authorization_engines_available;                                   // Array of authorization engines available.
pthread_cond_t sent_request_cond = PTHREAD_COND_INITIALIZER;            // Condition variable for sent request.
pthread_mutex_t sent_request_mutex = PTHREAD_MUTEX_INITIALIZER;         // Mutex for sent request.
int sent_request = 0;                                                   // Sent request.
int (*unnamed_pipes)[2];                                                // Array of unnamed pipes.
pid_t* authorization_engines;                                           // Array of authorization engines[AUTH_SERVERS];
/* ----------------------------------------------------------------------------------------------------------------------*/
int next_available_engine = 0;                                          // Next available engine.

/*-----------------------------------------------------------------------------------*/
/*                  AUTHORIZATION REQUESTS MANAGER                                   */
/*-----------------------------------------------------------------------------------*/

void auth_request_manager(){
    update_log("PROCESS AUTHORIZATION_REQUEST_MANAGER CREATED");

    // Initializes array of authorization engines available
    authorization_engines_available = calloc(AUTH_SERVERS+1, sizeof(int));
    for(int i = 0; i < AUTH_SERVERS; i++){
        authorization_engines_available[i] = 1;
    }

    // Initializes condition variable and mutex for engine available
    engine_available_cond = (pthread_cond_t *) malloc((AUTH_SERVERS + 1) * sizeof(pthread_cond_t));
    engine_available_mutex = (pthread_mutex_t *) malloc((AUTH_SERVERS + 1) * sizeof(pthread_mutex_t));

    for(int i = 0; i < AUTH_SERVERS; i++){
        pthread_cond_init(&engine_available_cond[i], NULL);
        pthread_mutex_init(&engine_available_mutex[i], NULL);
    }

    authorization_engines = (pid_t *) malloc((AUTH_SERVERS + 1) * sizeof(pid_t));
    // Creates unnamed pipes and authorizations engines
    unnamed_pipes = malloc(sizeof(int[AUTH_SERVERS+1][2]));
    for(int i = 0; i < AUTH_SERVERS; i++){
        pipe(unnamed_pipes[i]);
        if((authorization_engines[i] = fork()) == 0){
            authorization_engine(i);
        }
        close(unnamed_pipes[i][0]);
    }

    /* Creation of Video_Streaming_Queue and Others_Services_Queue*/
    video_queue = create_queue(QUEUE_POS);
    others_queue = create_queue(QUEUE_POS);

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
    pthread_create(&receiver_thread, NULL, receiver, NULL);
    pthread_create(&sender_thread, NULL, sender, NULL);
    
    // waits for them to die
    pthread_join(receiver_thread, NULL);
    pthread_join(sender_thread, NULL);

    // Closing unnamed pipes
    for(int i = 0; i < AUTH_SERVERS; i++)
        for(int j = 0; j < 2; j++)
            close(unnamed_pipes[i][j]);

    // Closing named pipes
    close(fd_user_pipe);
    close(fd_back_pipe);
    unlink(USER_PIPE);
    unlink(BACK_PIPE);

    exit(0);
}

/*-----------------------------------------------------------------------------------*/
/*                      MONITOR ENGINE and its functions                             */
/*-----------------------------------------------------------------------------------*/

void monitor_engine(){
    update_log("PROCESS MONITOR_ENGINE CREATED");

    send_data_periodically(10);

    while(1){
        send_plafond_alerts();
    }
    exit(0);
} 

void send_data_to_backoffice(){
    message_q message_to_send;
    message_to_send.type = 1; // Identifies that the message destination is the backoffice

    shared_var = attach_shared_memory(shmid);
    struct shm_system_data* data = (struct shm_system_data *) ((char *) shared_var + MOBILE_USERS * sizeof(struct shm_mobile_data));

    sprintf(message_to_send.message, "VIDEO-%d-%d\nSOCIAL-%d-%d\nMUSIC-%d-%d", data->video_requests, data->video_plafond_used, data->social_requests, data->social_plafond_used, data->music_requests, data->music_plafond_used);
    detach_shared_memory(shared_var);

    // Accessing message queue
    key_t key = ftok(MESSAGE_QUEUE, 'A');
    if(key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    msg_queue_id = msgget(key, IPC_CREAT | 0700);
    if(msg_queue_id == -1){
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Sends message
    if(msgsnd(msg_queue_id, &message_to_send, sizeof(message_to_send) - sizeof(long), 0) == -1){
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
    
    update_log("MONITOR_ENGINE: DATA STATS SENT TO BACKOFFICE");
}

void send_data_periodically(int interval){
    struct sigaction action;
    action.sa_handler = send_data_to_backoffice;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = interval;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = interval;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
}

void send_plafond_alerts(){
    key_t key = ftok(MESSAGE_QUEUE, 'A');
    if(key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int msg_queue_id = msgget(key, IPC_CREAT | 0700);
    if(msg_queue_id == -1){
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    
    shared_var = attach_shared_memory(shmid);
    struct shm_mobile_data* mobile_data = (struct shm_mobile_data *) shared_var;
    struct shm_system_data* system_data = (struct shm_system_data *) ((char *) shared_var + MOBILE_USERS * sizeof(struct shm_mobile_data));

    for(int i = 0; i < system_data->current_mobile_users; i++){
        /*
        *   Calculate remaining plafond ->  current_plafond  * 100
        *                                  -----------------
        *                                   initial_plafond
        */

        int remaining_plafond = ((float) mobile_data[i].current_plafond / mobile_data[i].initial_plafond) * 100;

        if(mobile_data[i].current_plafond < mobile_data[i].initial_plafond){
            if(remaining_plafond == 0 || remaining_plafond == 10 || remaining_plafond == 20){
                if(mobile_data[i].alert_sent[remaining_plafond / 10 - 1] == 1){
                    continue;
                }
                
                message_q message_to_send;
                message_to_send.type = mobile_data[i].mobile_id;
                sprintf(message_to_send.message, "ALERT: %d", remaining_plafond);
                if(msgsnd(msg_queue_id, &message_to_send, sizeof(message_q) - sizeof(long), 0) == -1){
                    detach_shared_memory(shared_var);
                    perror("msgsnd");
                    exit(EXIT_FAILURE);
                }
                mobile_data[i].alert_sent[remaining_plafond / 10 - 1] = 1;
                update_log("ALERT %d% (USER %d) TRIGGERED", 100 - remaining_plafond, mobile_data[i].mobile_id);

                // If remaining plafond is 0%, remove user from shared memory
                if(remaining_plafond == 0){
                    for(int j = i; j < system_data->current_mobile_users - 1; j++){
                        mobile_data[j] = mobile_data[j + 1];
                    }
                    system_data->current_mobile_users--;
                }
            }
        }
    }
    detach_shared_memory(shared_var);
}

/*-----------------------------------------------------------------------------------*/
/*                      RECEIVER THREAD and its functions                            */
/*-----------------------------------------------------------------------------------*/

void* receiver(){
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

    char* message_from_pipe = (char*) malloc(sizeof(char) * 100);
    if(message_from_pipe == NULL){
        fprintf(stderr, "Error malloc()\n");
        return NULL;
    }

    while(1){
        // I/O Multiplexing
		FD_ZERO(&read_set);
		FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if(select(max(fd_user_pipe, fd_back_pipe) + 1, &read_set, NULL, NULL, NULL) > 0){
            if(FD_ISSET(fd_user_pipe, &read_set)){
                struct request_t request;
                
                // Read from user_pipe
                strcpy(message_from_pipe, read_from_user_pipe());
                char* array[3] = {(char*) malloc(sizeof(char) * 10), (char*) malloc(sizeof(char) * 10), (char*) malloc(sizeof(char) * 10)};
                if(process_msg_from_user_pipe(message_from_pipe, array) == 0){
                    /*
                    * Request message
                    */

                    // Insert request in queue
                    request.mobile_id = atoi(array[0]);
                    strcpy(request.request_type, array[1]);
                    request.request_data = atoi(array[2]);
                    request.creation_time = time(NULL);

                    if(strcmp(array[1], "VIDEO") == 0){
                        if(insert(video_queue, request) == 0){
                            update_log("RECEIVER: VIDEO QUEUE IS FULL. REQUEST DROPPED");
                        }                    
                    } else{
                        // Insert in others queue
                        if(insert(others_queue, request) == 0){
                            update_log("RECEIVER: OTHERS QUEUE IS FULL. REQUEST DROPPED");
                        }
                    }
                } else{
                    /* 
                    *   Register message
                    */

                    // Insert request in queue
                    request.mobile_id = atoi(array[0]);
                    strcpy(request.request_type, "REGISTER");
                    request.request_data = atoi(array[1]); // request_data := initial_plafond
                    request.creation_time = time(NULL);     

                    if(insert(others_queue, request) == 0){
                        update_log("RECEIVER: OTHERS QUEUE IS FULL. REQUEST DROPPED");
                    }
                }

                pthread_mutex_lock(&sent_request_mutex);
                sent_request = 1;
                pthread_cond_signal(&sent_request_cond);
                pthread_mutex_unlock(&sent_request_mutex);

                free(array[0]);
                free(array[1]);
                free(array[2]);
                free(message_from_pipe);
                FD_CLR(fd_user_pipe, &read_set);
            }
            
            if(FD_ISSET(fd_back_pipe, &read_set)){
                // Read from back_pipe
                strcpy(message_from_pipe, read_from_back_pipe());
                char* array[2] = {(char*) malloc(sizeof(char) * 10), (char*) malloc(sizeof(char) * 10)};
                if(process_msg_from_back_pipe(message_from_pipe, array) == 1){
                    /*
                    * Backoffice message.
                    */
                    struct request_t request;
                    request.mobile_id = -1; // Identifies that it is a backoffice message
                    strcpy(request.request_type, array[1]);
                    request.request_data = -1; // Identifies that it is a backoffice message
                    request.creation_time = time(NULL);

                    // Insert in others queue
                    if(insert(others_queue, request) == 0){
                        update_log("RECEIVER: OTHERS QUEUE IS FULL. REQUEST DROPPED");
                    }

                    pthread_mutex_lock(&sent_request_mutex);
                    sent_request = 1;
                    pthread_cond_signal(&sent_request_cond);
                    pthread_mutex_unlock(&sent_request_mutex);
                }
                free(array[0]);
                free(array[1]);
                FD_CLR(fd_back_pipe, &read_set);
            }
        }
    }
    close(fd_user_pipe);
    close(fd_back_pipe);

    pthread_exit(NULL);
    return NULL;
}

int process_msg_from_user_pipe(char* message, char* array[]){
    char* token;
    token = strtok(message, "#");
    strcpy(array[0], token);
    token = strtok(NULL, "#");
    strcpy(array[1], token);
    token = strtok(NULL, "#");
    if(token == NULL){
        return 1;
    }
    strcpy(array[2], token);
    return 0;
}

int process_msg_from_back_pipe(char* message, char* array[]){
    char* token;
    token = strtok(message, "#");
    strcpy(array[0], token);
    token = strtok(NULL, "#");
    strcpy(array[1], token);
    return 1;
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

/*-----------------------------------------------------------------------------------*/
/*                              SENDER THREAD                                        */
/*-----------------------------------------------------------------------------------*/

void* sender(){
    update_log("THREAD SENDER CREATED");

    struct request_t request;

    while(1){
        /*
        *  Waiting for a request to be sent
        */
        pthread_mutex_lock(&sent_request_mutex);
        while(sent_request == 0){
            pthread_cond_wait(&sent_request_cond, &sent_request_mutex);
        }
        pthread_mutex_unlock(&sent_request_mutex);

        /*
        *  Waiting for a Authorization Engine to be available
        */

        int available_engine_id = -1;
        int start_index = next_available_engine;

        do{
            pthread_mutex_lock(&engine_available_mutex[next_available_engine]);
            if(authorization_engines_available[next_available_engine]){
                available_engine_id = next_available_engine;
                next_available_engine = (next_available_engine + 1) % AUTH_SERVERS; // Atualiza o índice para apontar para o próximo Authorization Engine disponível
                pthread_mutex_unlock(&engine_available_mutex[available_engine_id]);
                break;
            }
            pthread_mutex_unlock(&engine_available_mutex[next_available_engine]);
            next_available_engine = (next_available_engine + 1) % AUTH_SERVERS; // Atualiza o índice para a próxima iteração
        } while(next_available_engine != start_index);

        if(available_engine_id == -1){
            continue; // No available engine, continue waiting
        }


        /* 
        *   Authorization Engine available. Search for requests in the queues 
        *   Calling drop() function to remove the request from the queue and those
        *   that are waiting longer than the limit set by the System Manager.
        */


        if(!is_empty(video_queue)){
            request = drop(video_queue);
        } else if(!is_empty(others_queue)){
            request = drop(others_queue );
        } else{
            continue;
        }

        if(write(unnamed_pipes[available_engine_id][1], &request, sizeof(request)) == -1){ // Send message to authorization engine
            perror("write");
        }
        update_log("SENDER: %s AUTHORIZATION REQUEST (ID = %d) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", request.request_type, request.mobile_id, available_engine_id);
        
        pthread_mutex_lock(&sent_request_mutex);
        sent_request = 0;
        pthread_cond_signal(&sent_request_cond);
        pthread_mutex_unlock(&sent_request_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

/*-----------------------------------------------------------------------------------*/
/*                     AUTHORIZATION ENGINE and its functions                        */
/*-----------------------------------------------------------------------------------*/

void authorization_engine(int id){
    update_log("AUTHORIZATION ENGINE %d READY", id);

    close(unnamed_pipes[id][1]);

    struct request_t request; // Request received from sender
    struct timeval start_time, end_time; // Structs to calculate time

    while(1){
        ssize_t bytes_read = read(unnamed_pipes[id][0], &request, sizeof(request));
        if(bytes_read == -1){
            perror("read()");
        } else if(bytes_read == 0){
            continue;
        } else{
            // Authorization Engine is processing the request
            pthread_mutex_lock(&engine_available_mutex[id]);
            authorization_engines_available[id] = 0;
            pthread_cond_signal(&engine_available_cond[id]);
            pthread_mutex_unlock(&engine_available_mutex[id]);

            // Iniciar contagem de tempo
            gettimeofday(&start_time, NULL);

            if(request.mobile_id == -1 && request.request_data == -1){
                /*  
                *  --------------------
                *   Backoffice message.
                *  --------------------
                */
                if(strcmp(request.request_type, "data_stats") == 0){   
                    // Sends data stats to backoffice                 
                    send_data_to_backoffice();
                    update_log("AUTHORIZATION_ENGINE %d: %s AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id, request.request_type, request.mobile_id);
                } else if(strcmp(request.request_type, "reset") == 0){
                    // Resets the system data
                    shared_var = attach_shared_memory(shmid);

                    struct shm_system_data* data = (struct shm_system_data *) ((char *) shared_var + MOBILE_USERS * sizeof(struct shm_mobile_data));
                    
                    data->video_requests = 0;
                    data->video_plafond_used = 0;
                    data->social_requests = 0;
                    data->social_plafond_used = 0;
                    data->music_requests = 0;
                    data->music_plafond_used = 0;

                    detach_shared_memory(shared_var);

                    update_log("AUTHORIZATION_ENGINE %d: SYSTEM DATA RESET", id);
                }      
            } else{
                /*
                *  -------------------
                *   Request message.
                *  -------------------
                */
                shared_var = attach_shared_memory(shmid);

                struct shm_mobile_data* mobile_data = (struct shm_mobile_data *) shared_var;
                struct shm_system_data* system_data = (struct shm_system_data *) ((char *) shared_var + MOBILE_USERS * sizeof(struct shm_mobile_data));


                if(strcmp(request.request_type, "REGISTER") == 0){
                    /* Register Request*/
                    
                    struct shm_mobile_data* mobile_data = (struct shm_mobile_data *) shared_var;

                    mobile_data[system_data->current_mobile_users].mobile_id = request.mobile_id;
                    mobile_data[system_data->current_mobile_users].initial_plafond = request.request_data;
                    mobile_data[system_data->current_mobile_users].current_plafond = request.request_data;
                    for(int i = 0; i < 3; i++){
                        mobile_data[system_data->current_mobile_users].alert_sent[i] = 0;
                    }
                    system_data->current_mobile_users++;

                    print_memory(mobile_data, NULL);
                } else{
                    /* Authorization Request */
                    for(int i = 0; i < system_data->current_mobile_users; i++){
                        if(mobile_data[i].mobile_id == request.mobile_id){
                            if(mobile_data[i].current_plafond > 0){
                                mobile_data[i].current_plafond -= request.request_data;

                                printf("MOBILE %d: %d\n", mobile_data[i].mobile_id, mobile_data[i].current_plafond);
                                
                                if(strcmp(request.request_type, "VIDEO") == 0){
                                    system_data->video_requests++;
                                    system_data->video_plafond_used += request.request_data;
                                } else if(strcmp(request.request_type, "SOCIAL") == 0){
                                    system_data->social_requests++;
                                    system_data->social_plafond_used += request.request_data;
                                } else if(strcmp(request.request_type, "MUSIC") == 0){
                                    system_data->music_requests++;
                                    system_data->music_plafond_used += request.request_data;
                                }
                                print_memory(mobile_data, system_data);
                                break;
                            }
                          }
                    }
                }          
                detach_shared_memory(shared_var);
                update_log("AUTHORIZATION_ENGINE %d: %s AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id, request.request_type, request.mobile_id);
            }

            // Waiting for AUTH_PROC_TIME ms
            do {
                gettimeofday(&end_time, NULL);
            } while((end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec) < AUTH_PROC_TIME);
        
            pthread_mutex_lock(&engine_available_mutex[id]);
            authorization_engines_available[id] = 1;
            pthread_cond_signal(&engine_available_cond[id]);
            pthread_mutex_unlock(&engine_available_mutex[id]);
        }
    }
    exit(0);
}

void create_message_queue(){
    key_t key;
    if((key = ftok(MESSAGE_QUEUE, 65)) == -1){
        perror("Message Queue ftok()");
        return;
    }

    if((msg_queue_id = msgget(key, 0700 | IPC_CREAT)) == -1){
        perror("msgget");
        return;
    }
    return;
}

/*-----------------------------------------------------------------------------------*/
/*                            SYSTEM MANAGER FUNCTIONS                               */
/*-----------------------------------------------------------------------------------*/

void remove_message_queue(){
    if(msgctl(msg_queue_id, IPC_RMID, NULL) == -1){
        perror("msgctl");
        return;
    }
    return;
}

void handle_sigint(){
    printf("\n");
    update_log("SIGINT RECEIVED");

    remove_message_queue();
    destroy_queue(video_queue);
    destroy_queue(others_queue);
    free(unnamed_pipes);
    
    /*
    *   Remove Shared Memory
    */
    remove_shared_memory(shmid);


    /*
    *   Waiting for processes to be finished
    */
    //waitpid(auth_pid, NULL, 0);
    //waitpid(monitor_pid, NULL, 0);

    kill(0, SIGKILL);
}