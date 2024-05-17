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
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                          Global Variables                                                             */
/* ----------------------------------------------------------------------------------------------------------------------*/
pthread_t receiver_thread;                                              // Receiver thread.
pthread_t sender_thread;                                                // Sender thread.
pid_t* authorization_engines;                                           // Array of authorization engines[AUTH_SERVERS];
queue_t* video_queue;                                                   // Video_Streaming_Queue.
queue_t* others_queue;                                                  // Others_Services_Queue.
pthread_cond_t sent_request_cond = PTHREAD_COND_INITIALIZER;            // Condition variable for sent request.
int sent_request = 0;                                                   // Sent request.
pthread_mutex_t sent_request_mutex = PTHREAD_MUTEX_INITIALIZER;         // Mutex for sent request.
pthread_mutex_t video_queue_mutex = PTHREAD_MUTEX_INITIALIZER;          // Mutex for video queue.
pthread_mutex_t others_queue_mutex = PTHREAD_MUTEX_INITIALIZER;         // Mutex for others queue.
int (*unnamed_pipes)[2];                                                // Array of unnamed pipes.
int fd_user_pipe;                                                       // File descriptor for user pipe.
int fd_back_pipe;                                                       // File descriptor for back pipe.
int msg_queue_id;                                                       // Message queue id.
volatile sig_atomic_t sm_is_closed = 0;                                 // Variable to check if the system manager is closed.
volatile sig_atomic_t is_queue_full = 0;                                // Variable to check if one queue is full.
volatile sig_atomic_t queue_type = -1;                                  // Type of queue that is full. 0 -> video_queue, 1 -> others_queue.
volatile sig_atomic_t is_queue_by_half = 0;                             // Authorization Request Manager pid.
volatile sig_atomic_t is_arm_notified = 0;                              // Variable to check if the system manager is notified.
struct shm_mobile_data* mobile_data;                                    // Shared memory mobile data.
struct shm_system_data* system_data;                                    // Shared memory system data.
sem_t* msg_queue_sem;                                                   // Semaphore for message queue.                           
/* ----------------------------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------*/
/*                  AUTHORIZATION REQUESTS MANAGER                                   */
/*-----------------------------------------------------------------------------------*/

void auth_request_manager(){
    signal(SIGINT, handle_signal); // Signal handler for SIGINT
    
    update_log("PROCESS AUTHORIZATION_REQUEST_MANAGER CREATED");

    /* 
    *   Creation of unnamed pipes and authorizations engines 
    */

    authorization_engines = (pid_t *) malloc((AUTH_SERVERS + 1) * sizeof(pid_t));
    if(authorization_engines == NULL){
        update_log("AUTHORIZATION REQUEST MANAGER: ERROR ALLOCATING MEMORY");
        exit(EXIT_FAILURE);
    }

    unnamed_pipes = malloc(sizeof(int[AUTH_SERVERS+1][2]));
    if(unnamed_pipes == NULL){
        update_log("AUTHORIZATION REQUEST MANAGER: ERROR ALLOCATING MEMORY");
        exit(EXIT_FAILURE);
    }

    pid_t pid; // temporary variable to store the pid of the authorization engine
    for(int i = 0; i < AUTH_SERVERS; i++){
        pipe(unnamed_pipes[i]);
        if((pid = fork()) == 0){
            authorization_engine(i);
        }
        authorization_engines[i] = pid;
    }
    authorization_engines[AUTH_SERVERS] = 0; // Last element of the array is the backup engine
                                             // 0 means that the backup engine is not created yet

    shared_var = attach_shared_memory(shmid);
    system_data = (struct shm_system_data*) shared_var;
    mobile_data = (struct shm_mobile_data*)(system_data + 1);

    /* Initialize bitmap array for authorization engines */
    sem_wait(shm_sem);
    for(int i = 0; i < AUTH_SERVERS; i++){
        system_data->engines_available[i] = 1;
    }
    system_data->engines_available[AUTH_SERVERS] = 0;   // For now, the backup engine is not available.
    sem_post(shm_sem);

    /* Creation of Video_Streaming_Queue and Others_Services_Queue */
    video_queue = create_queue(QUEUE_POS);
    others_queue = create_queue(QUEUE_POS);

    /* Creation of user and back pipes if it doesn't exist yet */
	if((mkfifo(USER_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)){
		update_log("AUTHORIZATION REQUEST MANAGER: ERROR CREATING USER_PIPE");
        exit(EXIT_FAILURE);
	}
    if((mkfifo(BACK_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)){
        update_log("AUTHORIZATION REQUEST MANAGER: ERROR CREATING BACK_PIPE");
        exit(EXIT_FAILURE);
    }

    /*
    *   Create Receiver and Sender threads
    */
    pthread_create(&receiver_thread, NULL, receiver, NULL);
    pthread_create(&sender_thread, NULL, sender, NULL);

    while(!sm_is_closed){
        if(is_queue_full && !is_arm_notified){
            is_arm_notified = 1;

            // Create backup engine
            if(authorization_engines[AUTH_SERVERS] == 0){
                pipe(unnamed_pipes[AUTH_SERVERS]);
                if((pid = fork()) == 0){
                    authorization_engine(AUTH_SERVERS);
                }
                authorization_engines[AUTH_SERVERS] = pid;

                update_log("AUTHORIZATION REQUEST MANAGER: BACKUP ENGINE CREATED");

                sem_wait(shm_sem);
                system_data->engines_available[AUTH_SERVERS] = 1; // Backup engine is available to work
                sem_post(shm_sem);
            }
            is_queue_full = 0;
        }

        if(!is_queue_full && is_queue_by_half){
            is_queue_by_half = 0;

            // Remove unnamed pipe
            close(unnamed_pipes[AUTH_SERVERS][0]);
            close(unnamed_pipes[AUTH_SERVERS][1]);

            // Remove backup engine
            kill(authorization_engines[AUTH_SERVERS], SIGKILL);
            waitpid(authorization_engines[AUTH_SERVERS], NULL, 0);
            authorization_engines[AUTH_SERVERS] = 0;

            sem_wait(shm_sem);
            system_data->engines_available[AUTH_SERVERS] = 0;
            sem_post(shm_sem);

            update_log("AUTHORIZATION REQUEST MANAGER: BACKUP ENGINE REMOVED");

            is_queue_by_half = 0;
            is_queue_full = 0;
            is_arm_notified = 0;
        }
    }
    detach_shared_memory(shared_var);

    cleanup_resources();

    exit(0);
}

/*-----------------------------------------------------------------------------------*/
/*                      MONITOR ENGINE and its functions                             */
/*-----------------------------------------------------------------------------------*/

void monitor_engine(){
    signal(SIGINT, SIG_DFL); // Default signal handler for SIGINT

    update_log("PROCESS MONITOR_ENGINE CREATED");

    shared_var = attach_shared_memory(shmid);
    system_data = (struct shm_system_data*) shared_var;
    mobile_data = (struct shm_mobile_data*)(system_data + 1);

    /* 
    *   Creation of process responsible for send data periodically to backoffice 
    */
    if(fork() == 0){
        send_data_periodically();
        exit(0);
    }

    /* Monitor engine is responsible to send plafond alerts to mobile users */
    while(!sm_is_closed){
        send_plafond_alerts();
    }

    detach_shared_memory(shared_var);
    exit(0);
}

void send_data_to_backoffice(struct shm_system_data* system_data){
    message_q message_to_send;
    message_to_send.type = 1; // Identifies that the message destination is the backoffice

    // Message construction
    sem_wait(shm_sem);
    sprintf(message_to_send.message, "\nService\t\tTotal Data\tAuth Reqs\nVIDEO\t\t%d\t\t%d\nSOCIAL\t\t%d\t\t%d\nMUSIC\t\t%d\t\t%d\n", system_data->video_plafond_used, 
                                                                                                                                       system_data->video_requests, 
                                                                                                                                       system_data->social_plafond_used, 
                                                                                                                                       system_data->social_requests, 
                                                                                                                                       system_data->music_plafond_used, 
                                                                                                                                       system_data->music_requests);
    sem_post(shm_sem);

    /*
    *   Accessing message queue
    */

    key_t key = ftok(MESSAGE_QUEUE, 'A');
    if(key == -1){
        update_log("ERROR CREATING KEY (SENDIND DATA TO BACKOFFICE)");
        return;
    }

    msg_queue_id = msgget(key, IPC_CREAT | 0600);
    if(msg_queue_id == -1){
        update_log("ERROR GETTING MESSAGE QUEUE (SENDIND DATA TO BACKOFFICE)");
        return;
    }

    /*
    *   Send message to message queue
    */

    sem_wait(msg_queue_sem);
    if(msgsnd(msg_queue_id, &message_to_send, sizeof(message_to_send) - sizeof(long), 0) == -1){
        sem_post(msg_queue_sem);
        update_log("ERROR SENDING DATA TO BACKOFFICE");
        return;
    }
    sem_post(msg_queue_sem);

    return;
}

void send_data_periodically(){
    update_log("MONITOR ENGINE: SEND DATA PERIODICALLY PROCESS CREATED");

    signal(SIGINT, SIG_DFL); // Default signal handler for SIGINT
                             // When monitor engine is closed, the child process is also closed

    shared_var = attach_shared_memory(shmid);
    system_data = (struct shm_system_data*) shared_var;
    mobile_data = (struct shm_mobile_data*)(system_data + 1);

    while(1){
        sleep(30);
        send_data_to_backoffice(system_data);
    }
    detach_shared_memory(shared_var);
}

void send_plafond_alerts(){
    /*
    *   Accessing message queue
    */

    key_t key = ftok(MESSAGE_QUEUE, 'A');
    if(key == -1){
        update_log("MONITOR ENGINE: ERROR CREATING KEY (SENDING PLAFOND ALERTS)");
        exit(EXIT_FAILURE);
    }

    msg_queue_id = msgget(key, IPC_CREAT | 0600);
    if(msg_queue_id == -1){
        update_log("MONITOR ENGINE: ERROR GETTING MESSAGE QUEUE (SENDING PLAFOND ALERTS)");
        exit(EXIT_FAILURE);
    }

    sem_wait(shm_sem);

    if(system_data->current_mobile_users == 0){
        sem_post(shm_sem);
        return;
    }

    for(int i = 0; i < system_data->current_mobile_users; i++){
        /*
        *   Calculate remaining plafond ->  current_plafond  * 100
        *                                  -----------------
        *                                   initial_plafond
        */

        int remaining_plafond = ((float) mobile_data[i].current_plafond / mobile_data[i].initial_plafond) * 100;
        round_number(&remaining_plafond); // Round the number to 0, 10 or 20 if remaining_plafond is close to 0, 10 or 20

        if(remaining_plafond == 0 || remaining_plafond == 10 || remaining_plafond == 20){
            // Verify if the alert was already sent
            if(mobile_data[i].alert_sent[remaining_plafond / 10 - 1] == 1){
                continue;
            }

            /*
            *   Send alert to mobile user
            */

            message_q message_to_send;
            message_to_send.type = mobile_data[i].mobile_id;
            sprintf(message_to_send.message, "ALERT: %d", remaining_plafond);

            sem_wait(msg_queue_sem);
            if(msgsnd(msg_queue_id, &message_to_send, sizeof(message_q) - sizeof(long), 0) == -1){
                sem_post(msg_queue_sem);
                sem_post(shm_sem);
                update_log("MONITOR ENGINE: ERROR SENDING ALERT TO MOBILE USER");
                exit(EXIT_FAILURE);
            }
            sem_post(msg_queue_sem);

            update_log("ALERT %d% (USER %d) TRIGGERED", 100 - remaining_plafond, mobile_data[i].mobile_id);
            
            mobile_data[i].alert_sent[remaining_plafond / 10 - 1] = 1; // Update alert_sent array

            // If remaining plafond is 0%, remove user from shared memory
            if(remaining_plafond == 0){
                for(int j = i; j < system_data->current_mobile_users - 1; j++){
                    mobile_data[j] = mobile_data[j + 1];
                }
                system_data->current_mobile_users--;
            }
        }
    }
    sem_post(shm_sem);
}

void round_number(int* number){
    if(*number > 25){
        return;
    }

    if(*number > 15 && *number <= 25){
        *number = 20;
        return;
    }

    if(*number > 0 && *number <= 15){
        *number = 10;
        return;
    }

    *number = 0;
    return;
}

/*-----------------------------------------------------------------------------------*/
/*                      RECEIVER THREAD and its functions                            */
/*-----------------------------------------------------------------------------------*/

void* receiver(){
    update_log("THREAD RECEIVER CREATED");

    /* 
    *   Open named pipes for reading 
    */

	if((fd_back_pipe = open(BACK_PIPE, O_RDWR)) < 0){
		update_log("RECEIVER: ERROR OPENING BACK_PIPE");
        return NULL;
	}

    if((fd_user_pipe = open(USER_PIPE, O_RDWR)) < 0){
		update_log("RECEIVER: ERROR OPENING USER_PIPE");
        return NULL;
	}

    char* message_from_pipe = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message_from_pipe == NULL){
        update_log("RECEIVER: ERROR ALLOCATING MEMORY");
        return NULL;
    }

    fd_set read_set;
    while(1){
		FD_ZERO(&read_set);
		FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if(select(max(fd_user_pipe, fd_back_pipe) + 1, &read_set, NULL, NULL, NULL) > 0){
            /*
            *   Message received from user_pipe
            */
            if(FD_ISSET(fd_user_pipe, &read_set)){
                struct request_t request; // Request to send to thread sender

                strcpy(message_from_pipe, read_from_user_pipe()); // Read from user_pipe

                // array to store the message from the pipe
                char* array[3] = {(char*) malloc(sizeof(char) * 10),
                                  (char*) malloc(sizeof(char) * 10),
                                  (char*) malloc(sizeof(char) * 10)};

                if(process_msg_from_user_pipe(message_from_pipe, array) == 0){
                    /*
                    *   *------------------------*
                    *   * Mobile request message *
                    *   *------------------------*
                    */

                    /*
                    *   Insert request in queue
                    */

                    request.mobile_id = atoi(array[0]);
                    strcpy(request.request_type, array[1]);
                    request.request_data = atoi(array[2]);

                    struct timeval current_time; // Struct to store the current time of the request
                    gettimeofday(&current_time, NULL);
                    request.creation_time = current_time;

                    if(strcmp(array[1], "VIDEO") == 0){
                        // Insert in video queue
                        pthread_mutex_lock(&video_queue_mutex);
                        if(insert(video_queue, request) == 0){
                            update_log("RECEIVER: VIDEO QUEUE IS FULL - REQUEST DROPPED");
                        }
                        pthread_mutex_unlock(&video_queue_mutex);
                    } else{
                        // Insert in others queue
                        pthread_mutex_lock(&others_queue_mutex);
                        if(insert(others_queue, request) == 0){
                            update_log("RECEIVER: OTHERS QUEUE IS FULL - REQUEST DROPPED");
                        }
                        pthread_mutex_unlock(&others_queue_mutex);
                    }
                } else{
                    /*
                    *   *-------------------------*
                    *   * Mobile register message *
                    *   *-------------------------*
                    *   
                    *   
                    *   We assumed that a Mobile Register Message needs to be prioritized
                    *   over a normal request message so we insert it in the videos queue.
                    */

                    /*
                    *   Insert request in queue
                    */

                    request.mobile_id = atoi(array[0]);
                    strcpy(request.request_type, "REGISTER");
                    request.request_data = atoi(array[1]); // request_data := initial_plafond

                    struct timeval current_time; // Struct to store the current time of the request
                    gettimeofday(&current_time, NULL);
                    request.creation_time = current_time;

                    pthread_mutex_lock(&video_queue_mutex);
                    if(insert(video_queue, request) == 0){
                        update_log("RECEIVER: VIDEO QUEUE IS FULL - REQUEST DROPPED");
                    }
                    pthread_mutex_unlock(&video_queue_mutex);
                }

                /*
                *   Signal sender thread that a request was sent
                */

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

            /*
            *   Message received from back_pipe
            */
            if(FD_ISSET(fd_back_pipe, &read_set)){
                strcpy(message_from_pipe, read_from_back_pipe());

                // array to store the message from the pipe
                char* array[2] = {(char*) malloc(sizeof(char) * 10),
                                  (char*) malloc(sizeof(char) * 10)};

                if(process_msg_from_back_pipe(message_from_pipe, array) == 1){
                    /*
                    *   *--------------------*
                    *   * Backoffice message *
                    *   *--------------------*
                    */

                    struct request_t request; // Request to send to thread sender

                    request.mobile_id = 1; // Identifies that it is a backoffice message
                    strcpy(request.request_type, array[1]);
                    request.request_data = 1; // Identifies that it is a backoffice message

                    struct timeval current_time; // Struct to store the current time of the request
                    gettimeofday(&current_time, NULL);
                    request.creation_time = current_time;

                    // Insert in others queue
                    pthread_mutex_lock(&others_queue_mutex);
                    if(insert(others_queue, request) == 0){
                        update_log("RECEIVER: OTHERS QUEUE IS FULL. REQUEST DROPPED");
                    }
                    pthread_mutex_unlock(&others_queue_mutex);
                }

                // Signal sender thread that a request was sent
                pthread_mutex_lock(&sent_request_mutex);
                sent_request = 1;
                pthread_cond_signal(&sent_request_cond);
                pthread_mutex_unlock(&sent_request_mutex);

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
    char* token = strtok(message, "#");
    if(token == NULL){
        free(token);
        return -1;
    }
    strcpy(array[0], token);

    token = strtok(NULL, "#");
    if(token == NULL){
        free(token);
        return -1;
    }
    strcpy(array[1], token);

    token = strtok(NULL, "#");
    if(token == NULL){
        free(token);
        return 1; // is a register message
    }
    strcpy(array[2], token);
    return 0; // is a request message
}

int process_msg_from_back_pipe(char* message, char* array[]){
    char* token = strtok(message, "#");
    if(token == NULL){
        return -1;
    }
    strcpy(array[0], token);

    token = strtok(NULL, "#");
    if(token == NULL){
        return -1;
    }
    strcpy(array[1], token);
    return 1; // is a backoffice message
}

char* read_from_user_pipe(){
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        update_log("RECEIVER: ERROR ALLOCATING MEMORY");
        return NULL;
    }

    if(read(fd_user_pipe, message, MESSAGE_SIZE) == -1){
        update_log("RECEIVER: ERROR READING FROM USER_PIPE");
        return NULL;
    }
    return message;
}

char* read_from_back_pipe(){
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        update_log("RECEIVER: ERROR ALLOCATING MEMORY");
        return NULL;
    }

    if(read(fd_back_pipe, message, MESSAGE_SIZE) == -1){
        update_log("RECEIVER: ERROR READING FROM BACK_PIPE");
        return NULL;
    }
    return message;
}

/*-----------------------------------------------------------------------------------*/
/*                              SENDER THREAD                                        */
/*-----------------------------------------------------------------------------------*/

void* sender(){
    update_log("THREAD SENDER CREATED");

    shared_var = attach_shared_memory(shmid);
    system_data = (struct shm_system_data*) shared_var;
    mobile_data = (struct shm_mobile_data*)(system_data + 1);

    struct request_t request; // Request to send to authorization engine

    while(1){
        /*
        *   Waiting for a request to be sent
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

        sem_wait(shm_sem);
        while(available_engine_id == -1){
            for(int i = 0; i < AUTH_SERVERS + 1; i++){
                if(system_data->engines_available[i] == 1){
                    available_engine_id = i;
                    break;
                }
            }
        }
        sem_post(shm_sem);

        /*
        *   Authorization Engine available. Search for requests in the queues(first in video_queue
        *   because it has priority over others_queue) calling drop() function to remove the request
        *   from the queue and those that are waiting longer than the limit set by the System Manager.
        *   Sender also checks if the queues are full and sets the is_queue_full variable to 1 to
        *   Authorization Request Manager create a backup engine.
        */


        // If one of the queues is full, notify the Authorization Request Manager
        if((is_full(video_queue) || is_full(others_queue)) && !is_arm_notified){
            is_queue_full = 1; // Set is_queue_full to 1 to create a backup engine

            queue_type = is_full(video_queue) ? 0 : 1; // save the type of queue that is full
            continue;
        } else if(!is_empty(video_queue)){
            pthread_mutex_lock(&video_queue_mutex);
            request = drop(video_queue);
            if(request.mobile_id == 0){
                pthread_mutex_unlock(&video_queue_mutex);
                continue;
            }
            pthread_mutex_unlock(&video_queue_mutex);
        } else if(!is_empty(others_queue)){
            pthread_mutex_lock(&others_queue_mutex);
            request = drop(others_queue);
            if(request.mobile_id == 0){
                pthread_mutex_unlock(&others_queue_mutex);
                continue;
            }
            pthread_mutex_unlock(&others_queue_mutex);
        } else{
            continue;
        }

        // If Authorization Request Manager is notified that one of the queues is full, check if the queues are by half.
        // If the queues are by half, notify the Authorization Request Manager.
        if(((queue_type == 0 && is_queue_half_full(video_queue)) || (queue_type == 1 && is_queue_half_full(others_queue))) && is_arm_notified){
            is_queue_by_half = 1;
        }

        // Send message to authorization engine
        if(write(unnamed_pipes[available_engine_id][1], &request, sizeof(request)) == -1){ 
            update_log("SENDER: ERROR WRITING TO AUTHORIZATION ENGINE");
            pthread_mutex_lock(&sent_request_mutex);
            sent_request = 0;
            pthread_cond_signal(&sent_request_cond);
            pthread_mutex_unlock(&sent_request_mutex);
            return NULL;
        }

        update_log("SENDER: %s AUTHORIZATION REQUEST (ID = %d) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", request.request_type, request.mobile_id, available_engine_id);

        pthread_mutex_lock(&sent_request_mutex);
        sent_request = 0;
        pthread_cond_signal(&sent_request_cond);
        pthread_mutex_unlock(&sent_request_mutex);
    }

    detach_shared_memory(shared_var);
    pthread_exit(NULL);
    return NULL;
}

/*-----------------------------------------------------------------------------------*/
/*                     AUTHORIZATION ENGINE and its functions                        */
/*-----------------------------------------------------------------------------------*/

void authorization_engine(int id){
    update_log("AUTHORIZATION ENGINE %d READY", id);

    /*
    *  Signal handler for SIGUSR1 and SIGINT
    */
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGINT, SIG_IGN); // Ignore SIGINT signal receibed by authorization requests manager
                             // to complete the requests that are being processed


    close(unnamed_pipes[id][1]); // Close write end of the pipe

    shared_var = attach_shared_memory(shmid);
    system_data = (struct shm_system_data*) shared_var;
    mobile_data = (struct shm_mobile_data*)(system_data + 1);

    struct request_t request;  // Request received from sender
    struct timeval start_time; // Struct to calculate time

    // Usage of select() to read from pipe:
    //       - Avoid blocking the process and cannot leave when sm_is_closed == 1
    //       - Timeout of 5 seconds to check if the process should be closed

    fd_set read_fds;
    struct timeval timeout = {
        .tv_sec = 5,
        .tv_usec = 0
    };
    int retval;

    while(!sm_is_closed){
        FD_ZERO(&read_fds);
        FD_SET(unnamed_pipes[id][0], &read_fds);

        retval = select(unnamed_pipes[id][0] + 1, &read_fds, NULL, NULL, &timeout);

        if(retval == -1){
            break; // Error in select()
        } else if(retval){
            /* There is data on unnamed pipe */
            ssize_t bytes_read = read(unnamed_pipes[id][0], &request, sizeof(request));
            if(bytes_read == -1){
                update_log("AUTHORIZATION_ENGINE %d: ERROR READING UNNAMED PIPE"); // Error in read()
                break;
            } else if(bytes_read == 0){
                continue; // No data to read
            } else{
                /*
                *   Authorization Engine received a request
                */

                sem_wait(shm_sem);
                system_data->engines_available[id] = 0; // Authorization Engine is processing the request
                sem_post(shm_sem);

                gettimeofday(&start_time, NULL); // Start time to calculate processing time

                if(request.mobile_id == 1 && request.request_data == 1){
                    /*
                    *   *--------------------*
                    *   * Backoffice message *
                    *   *--------------------*
                    */

                    if(strcmp(request.request_type, "data_stats") == 0){
                        /* 
                        *   Send data stats to backoffice 
                        */

                        send_data_to_backoffice(system_data);

                        // Waiting for AUTH_PROC_TIME ms
                        wait_for_processing_time(start_time);

                        update_log("AUTHORIZATION_ENGINE %d: %s AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id, request.request_type, request.mobile_id);
                    } else if(strcmp(request.request_type, "reset") == 0){
                        /* 
                        *   Resets the system data 
                        */

                        sem_wait(shm_sem);

                        system_data->video_requests = 0;
                        system_data->video_plafond_used = 0;
                        system_data->social_requests = 0;
                        system_data->social_plafond_used = 0;
                        system_data->music_requests = 0;
                        system_data->music_plafond_used = 0;

                        sem_post(shm_sem);

                        // Waiting for AUTH_PROC_TIME ms
                        wait_for_processing_time(start_time);

                        update_log("AUTHORIZATION_ENGINE %d: SYSTEM DATA RESET", id);
                        update_log("AUTHORIZATION_ENGINE %d: %s AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id, request.request_type, request.mobile_id);
                    }
                } else{
                    /*
                    *   *----------------*
                    *   * Mobile message *
                    *   *----------------*
                    */

                    sem_wait(shm_sem);

                    if(strcmp(request.request_type, "REGISTER") == 0){
                        /* 
                        *   Register Request 
                        */

                        mobile_data[system_data->current_mobile_users].mobile_id = request.mobile_id;
                        mobile_data[system_data->current_mobile_users].initial_plafond = request.request_data;
                        mobile_data[system_data->current_mobile_users].current_plafond = request.request_data;
                        for(int i = 0; i < 3; i++){
                            mobile_data[system_data->current_mobile_users].alert_sent[i] = 0;
                        }

                        kill(request.mobile_id, SIGUSR1); // Notify mobile that it is registered

                        system_data->current_mobile_users++; // Increment the number of mobile users
                    } else{
                        /* 
                        *   Authorization Request 
                        */

                        for(int i = 0; i < system_data->current_mobile_users; i++){
                            if(mobile_data[i].mobile_id == request.mobile_id){
                                // Mobile found

                                if(mobile_data[i].current_plafond <= 0){
                                    break; // No plafond available
                                }
                                
                                /*
                                *   Decrease plafond and update system data
                                */
                                 
                                if(mobile_data[i].current_plafond - request.request_data < 0){
                                    update_log("AUTHORIZATION_ENGINE %d: %s AUTHORIZATION REQUEST (ID = %d) DENIED - INSUFFICIENT PLAFOND", id, request.request_type, request.mobile_id);
                                    break; // No plafond available
                                }
                                
                                mobile_data[i].current_plafond -= request.request_data;

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
                                break; // Mobile found and plafond updated, leave.
                            }
                        }
                    }
                    sem_post(shm_sem);

                    // Waiting for AUTH_PROC_TIME ms
                    wait_for_processing_time(start_time);

                    update_log("AUTHORIZATION_ENGINE %d: %s AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id, request.request_type, request.mobile_id);
                }

                sem_wait(shm_sem);
                system_data->engines_available[id] = 1; // Authorization Engine is available
                sem_post(shm_sem);
            }
        } else{
            continue;
        }
    }
    detach_shared_memory(shared_var);
    exit(0);
}

void wait_for_processing_time(struct timeval start_time){
    struct timeval end_time; // Struct to store the end time of the request
    gettimeofday(&end_time, NULL);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    long remaining_time = AUTH_PROC_TIME * 1000 - elapsed_time;

    if(remaining_time > 0){
        usleep(remaining_time);
    }
}

void sigusr1_handler(int signal){
    if(signal == SIGUSR1){
        sm_is_closed = 1;
    }
}

/*-----------------------------------------------------------------------------------*/
/*                            SYSTEM MANAGER FUNCTIONS                               */
/*-----------------------------------------------------------------------------------*/

void create_message_queue(){
    sem_unlink(SEM_MSG_QUEUE); // Unlinks semaphore (if it exists)
    msg_queue_sem = sem_open(SEM_MSG_QUEUE, O_CREAT | O_EXCL, 0600, 1);
    if(msg_queue_sem == SEM_FAILED){
        update_log("ERROR CREATING SEMAPHORE (MESSAGE QUEUE)");
        exit(EXIT_FAILURE);
    }

    key_t key;
    if((key = ftok(MESSAGE_QUEUE, 65)) == -1){
        update_log("ERROR CREATING KEY (MESSAGE QUEUE)");
        return;
    }

    if((msg_queue_id = msgget(key, 0600 | IPC_CREAT)) == -1){
        update_log("ERROR CREATING MESSAGE QUEUE");
        return;
    }
    return;
}

void remove_message_queue(){
    sem_close(msg_queue_sem);
    sem_unlink(SEM_MSG_QUEUE);

    key_t key;
    if((key = ftok(MESSAGE_QUEUE, 65)) == -1){
        update_log("ERROR CREATING KEY (MESSAGE QUEUE)");
        return;
    }

    int msg_queue_id = msgget(key, IPC_CREAT | 0600);
    if(msg_queue_id == -1){
        update_log("ERROR GETTING MESSAGE QUEUE (REMOVE)");
        exit(EXIT_FAILURE);
    }

    if(msgctl(msg_queue_id, IPC_RMID, NULL) == -1){
        update_log("ERROR REMOVING MESSAGE QUEUE");
        return;
    }
    return;
}

void cleanup_resources(){
    remove_message_queue();
    update_log("MESSAGE QUEUE REMOVED");

    remove_shared_memory(shmid);
    update_log("SHARED MEMORY REMOVED");

    destroy_queue(video_queue);
    destroy_queue(others_queue);
    update_log("QUEUES DESTROYED");

    for(int i = 0; i < AUTH_SERVERS; i++){
        close(unnamed_pipes[i][0]);
        close(unnamed_pipes[i][1]);
    }
    if(unnamed_pipes != NULL) free(unnamed_pipes);
    update_log("UNNAMED PIPES CLOSED");

    close(fd_user_pipe);
    close(fd_back_pipe);
    unlink(USER_PIPE);
    unlink(BACK_PIPE);
    update_log("NAMED PIPES CLOSED");

    if(authorization_engines != NULL) free(authorization_engines);
}

void handle_signal(int signal){
    if(signal == SIGINT){
        /*
        *   Cancel receiver and sender threads
        */
        pthread_cancel(receiver_thread);
        pthread_cancel(sender_thread);

        pthread_join(receiver_thread, NULL);
        pthread_join(sender_thread, NULL);

        update_log("THREADS RECEIVER AND SENDER CANCELED");

        /*
        *   Waiting for last requests to be processed
        */

        // Signaling authorization engines to finish
        for(int i = 0; i <= AUTH_SERVERS; i++){
            if(authorization_engines[i] != 0){
                kill(authorization_engines[i], SIGUSR1);
            }
        }

        update_log("5G_AUTH_PLATFORM SIMULATOR WAITING FOR LAST TASKS TO FINISH");

        // Waiting for authorization engines to finish
        for(int i = 0; i <= AUTH_SERVERS; i++){
            if(authorization_engines[i] != 0){
                waitpid(authorization_engines[i], NULL, 0);
            }
        }

        // Print requests that were not processed
        queue_node* aux = video_queue->head;
        while(aux != NULL){
            update_log("%s REQUEST (ID = %d) NOT PROCESSED", aux->request.request_type, aux->request.mobile_id);
            aux = aux->next;
        }

        aux = others_queue->head;
        while(aux != NULL){
            update_log("%s REQUEST (ID = %d) NOT PROCESSED", aux->request.request_type, aux->request.mobile_id);
            aux = aux->next;
        }

        // Cleanup resources
        cleanup_resources();

        exit(0); // Authorizations Requests Manager process ends
    }
}