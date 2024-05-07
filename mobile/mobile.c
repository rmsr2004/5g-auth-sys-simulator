// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "../globals/globals.h"
#include "mobile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

time_t last_social_request = 0;
time_t last_music_request = 0;
time_t last_video_request = 0;
char* message;

int main(int argc, char* argv[]){
    /*
    *   Checks if Mobile has been run correctly
    */
    if(argc != 7){
        fprintf(stderr, "Invalide Run\nUsage: %s {plafond inicial} / {número máximo de pedidos de autorização} /\n {intervalo VIDEO} / {intervalo MUSIC} / {intervalo SOCIAL} / {dados a reservar}\n", argv[0]);
        return -1;
    }

    for(int i = 1; i < argc; i++){
        // Checks if the argument are number
        if(!is_number(argv[i])){
            fprintf(stderr, "'%s' must be a number\n", argv[i]);
            return -1;
        }
        // Checks if the argument are positive
        if(atoi(argv[i]) <= 0){
            fprintf(stderr, "'%s' must be > 0\n", argv[i]);
            return -1;
        }
    }
    
    int INITIAL_PLAFOND   = atoi(argv[1]),
        MAX_AUTH_REQUESTS = atoi(argv[2]),
        VIDEO_GAP         = atoi(argv[3]),
        MUSIC_GAP         = atoi(argv[4]),
        SOCIAL_GAP        = atoi(argv[5]),
        MOBILE_DATA_USE   = atoi(argv[6]);
    
    pid_t mobile_id = getpid();

    signal(SIGINT, handle_sigint);

    // Create registration message to send to user pipe.
    message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        fprintf(stderr, "Error malloc()\n");
        printf("Resources cleaned. Exiting...\n");
        exit(0);
        return -1;
    }

    sprintf(message, "%d#%d", mobile_id, INITIAL_PLAFOND);
    write_to_user_pipe(message);
    
    // Work from mobile user

    int auth_requests = 0;
    srand(time(NULL));

    while(auth_requests < MAX_AUTH_REQUESTS){
        char* message = read_message_queue();
        if(message != NULL){
            printf("Received message from message queue: %s\n", message);
            if(process_message_from_queue(message)){
                free(message);
                clean_resources();
                return 0;
            }
            free(message);
        }
        
        int random_number = generate_random_number(1, 15);

        if(random_number <= 5){
            if(can_make_request(last_video_request, VIDEO_GAP)){
                message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
                if(message == NULL){
                    fprintf(stderr, "Error malloc()\n");
                    clean_resources();
                    return -1;
                }

                time_t current_time = time(NULL);
                char* time_str = ctime(&current_time);
                if(time_str[strlen(time_str)-1] == '\n') {
                    time_str[strlen(time_str)-1] = '\0'; // Removing newline character
                }

                //printf("%s-%d#VIDEO#%d\n", time_str, mobile_id, MOBILE_DATA_USE);
                
                sprintf(message, "%d#VIDEO#%d", mobile_id, MOBILE_DATA_USE);
                write_to_user_pipe(message);
                
                last_video_request = time(NULL);
                auth_requests++;
            }
        } else if(random_number <= 10){
            if(can_make_request(last_music_request, MUSIC_GAP)){
                message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
                if(message == NULL){
                    fprintf(stderr, "Error malloc()\n");
                    clean_resources();
                    return -1;
                }

                time_t current_time = time(NULL);
                char* time_str = ctime(&current_time);
                if(time_str[strlen(time_str)-1] == '\n') {
                    time_str[strlen(time_str)-1] = '\0'; // Removing newline character
                }

                //printf("%s-%d#MUSIC#%d\n", time_str, mobile_id, MOBILE_DATA_USE);

                sprintf(message, "%d#MUSIC#%d", mobile_id, MOBILE_DATA_USE);
                write_to_user_pipe(message);
                
                last_music_request = time(NULL);
                auth_requests++;
            }
        } else{
            if(can_make_request(last_social_request, SOCIAL_GAP)){
                message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
                if(message == NULL){
                    fprintf(stderr, "Error malloc()\n");
                    clean_resources();
                    return -1;
                }

                time_t current_time = time(NULL);
                char* time_str = ctime(&current_time);
                if(time_str[strlen(time_str)-1] == '\n') {
                    time_str[strlen(time_str)-1] = '\0'; // Removing newline character
                }

                //printf("%s-%d#SOCIAL#%d\n", time_str, mobile_id, MOBILE_DATA_USE);
                
                sprintf(message, "%d#SOCIAL#%d", mobile_id, MOBILE_DATA_USE);
                write_to_user_pipe(message);
                
                last_social_request = time(NULL);
                auth_requests++;
            }
        }
    }
    clean_resources();
    return 0;
}

void handle_sigint(){
    printf("\nCTRL+C pressed. ");
    clean_resources();
}

void clean_resources(){
    printf("Cleaning resources...\n");
    // Close user_pipe
    close(fd_user_pipe);

    // Free message
    if(message != NULL) free(message);
        
    printf("Resources cleaned. Exiting...\n");
    exit(0);
}

void write_to_user_pipe(char* message){
    fd_user_pipe = open(USER_PIPE, O_WRONLY | O_NONBLOCK);
    write(fd_user_pipe, message, strlen(message) + 1);
    close(fd_user_pipe);
}

int can_make_request(time_t last_request_time, int interval){
    time_t current_time = time(NULL);
    return (current_time - last_request_time >= interval);
}

char* read_message_queue(){
    message_q received_message;
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);

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

    int my_id = getpid();
    ssize_t bytes_read = msgrcv(msg_queue_id, &received_message, sizeof(received_message) - sizeof(long), my_id, IPC_NOWAIT);
    if(bytes_read == -1){
        if(errno == ENOMSG){
            return NULL;
        } else {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
    }

    received_message.message[bytes_read] = '\0';
    strcpy(message, received_message.message);
    return message;
}

int process_message_from_queue(char* message){
    char* token = strtok(message, " ");
    token = strtok(NULL, " ");
    if(atoi(token) == 0){
        return 1;
    }
    return 0;
}

int generate_random_number(int min, int max){
    return rand() % (max - min + 1) + min;
}