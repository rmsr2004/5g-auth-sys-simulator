// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "mobile.h"
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

/*********************************************************************************************************
*                                              MOBILE                                                    *                                           
**********************************************************************************************************
*   Syntax of the Mobile User process initialisation command:                                            *
*       $ mobile_user / {initial pool} / {maximum number of authorisation requests} /                    *     
*        {VIDEO range} {MUSIC range} {SOCIAL range} / {data to reserve}                                  *
*                                                                                                        *
*   Information to send to the named pipe for the registration message:                                  * 
*       ID_mobile_user#Initial plafond                                                                   *   
*                                                                                                        * 
*   Information to send to the named pipe for the authorisation request message:                         * 
*       ID_mobile_user#Service ID#Amount of data to reserve                                              *
*                                                                                                        *                                     
*   The Mobile_User identifier, corresponding to the PID, will be used to group the                      *       
*   user's information in the shared memory.                                                             *
*   The Mobile_User receives alerts about the data ceiling (80%, 90%, 100%) via the Message Queue.       *
*   The Mobile User process ends when one of the following conditions is met:                            *
*       - 1. A SIGINT signal is received;                                                                *
*       - 2. Receipt of a 100 per cent data cap alert;                                                   *      
*       - 3. If the maximum number of authorisation requests is reached;                                 *   
*       - 4. In the event of an error - an error can occur if a parameter is wrong or if you try to      *
*            write to the named pipe and the write fails. In these cases you should write the error      *
*            message on the screen.                                                                      *
*********************************************************************************************************/

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                          Global Variables                                                             */
/* ----------------------------------------------------------------------------------------------------------------------*/
time_t last_social_request = 0;     // Last time a social request was made.
time_t last_music_request = 0;      // Last time a music request was made.
time_t last_video_request = 0;      // Last time a video request was made.
int logged = 0;                     // Flag to check if mobile has been logged.
char* message;                      // Variable to store messages.
int fd_user_pipe;                   // File descriptor for user pipe.
pid_t child_pid;                    // Child process pid.
/* ----------------------------------------------------------------------------------------------------------------------*/

int main(int argc, char* argv[]){
    signal(SIGINT, handle_signal);  // Set signal handler for SIGINT
    signal(SIGUSR1, handle_signal); // Set signal handler for SIGUSR1
    signal(SIGUSR2, handle_signal); // Set signal handler for SIGUSR2

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
    
    int INITIAL_PLAFOND   = atoi(argv[1]);      // Initial mobile plafond.
    int MAX_AUTH_REQUESTS = atoi(argv[2]);      // Maximum number of authorization requests mobile can do.
    int VIDEO_GAP         = atoi(argv[3]);      // Time interval between video requests.
    int MUSIC_GAP         = atoi(argv[4]);      // Time interval between music requests.
    int SOCIAL_GAP        = atoi(argv[5]);      // Time interval between social requests.
    int MOBILE_DATA_USE   = atoi(argv[6]);      // Data usage for each request.
    
    pid_t my_id = getpid(); // Mobile user id.

    // Create registration message to send to user pipe.
    message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        perror("malloc()");
        kill(getpid(), SIGUSR2);
    }

    /*
    *  Sends registration message to user pipe.
    */

    sprintf(message, "%d#%d", my_id, INITIAL_PLAFOND);
    write_to_user_pipe(message);

    // Wait for system manager to accept mobile registration
    while(!logged){}

    if((child_pid = fork()) == 0){
        child_process();
        exit(0);
    }

    int auth_requests = 0;
    srand(time(NULL));

    while(auth_requests < MAX_AUTH_REQUESTS){
        int random_number = generate_random_number(1, 15);

        if(random_number <= 5){
            if(can_make_request(last_video_request, VIDEO_GAP)){
                message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
                if(message == NULL){
                    perror("malloc()");
                    kill(getpid(), SIGUSR2);
                }

                time_t current_time = time(NULL);
                char* time_str = ctime(&current_time);
                if(time_str[strlen(time_str)-1] == '\n') {
                    time_str[strlen(time_str)-1] = '\0'; // Removing newline character
                }

                printf("%s-%d#VIDEO#%d\n", time_str, my_id, MOBILE_DATA_USE);
                
                sprintf(message, "%d#VIDEO#%d", my_id, MOBILE_DATA_USE);
                write_to_user_pipe(message);
                
                last_video_request = time(NULL);
                auth_requests++;
            }
        } else if(random_number <= 10){
            if(can_make_request(last_music_request, MUSIC_GAP)){
                message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
                if(message == NULL){
                    perror("malloc()");
                    kill(getpid(), SIGUSR2);
                }

                time_t current_time = time(NULL);
                char* time_str = ctime(&current_time);
                if(time_str[strlen(time_str)-1] == '\n') {
                    time_str[strlen(time_str)-1] = '\0'; // Removing newline character
                }

                printf("%s-%d#MUSIC#%d\n", time_str, my_id, MOBILE_DATA_USE);

                sprintf(message, "%d#MUSIC#%d", my_id, MOBILE_DATA_USE);
                write_to_user_pipe(message);
                
                last_music_request = time(NULL);
                auth_requests++;
            }
        } else{
            if(can_make_request(last_social_request, SOCIAL_GAP)){
                message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
                if(message == NULL){
                    perror("malloc()");
                    kill(getpid(), SIGUSR2);
                }

                time_t current_time = time(NULL);
                char* time_str = ctime(&current_time);
                if(time_str[strlen(time_str)-1] == '\n') {
                    time_str[strlen(time_str)-1] = '\0'; // Removing newline character
                }

                printf("%s-%d#SOCIAL#%d\n", time_str, my_id, MOBILE_DATA_USE);
                
                sprintf(message, "%d#SOCIAL#%d", my_id, MOBILE_DATA_USE);
                write_to_user_pipe(message);
                
                last_social_request = time(NULL);
                auth_requests++;
            }
        }
    }
    printf("MAXIMUM NUMBER OF AUTHORIZATION REQUESTS REACHED\n");
    clean_resources();
    return 0;
}

void child_process(){
    signal(SIGINT, SIG_DFL); // Set default signal handler for child process

    while(1){
        char* message = read_message_queue();
        if(message != NULL){
            if(strcmp(message, "-1") == 0){
                kill(getppid(), SIGUSR2);
            }

            printf("%s\n", message);
            if(process_message_from_queue(message)){
                free(message);
                clean_resources();
                return;
            }
            free(message);
        }
    }
}

void write_to_user_pipe(char* message){ 
    fd_user_pipe = open(USER_PIPE, O_WRONLY | O_NONBLOCK);
    write(fd_user_pipe, message, strlen(message) + 1);
    close(fd_user_pipe);
}

char* read_message_queue(){
    message_q received_message;
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        perror("malloc()");
        kill(getppid(), SIGUSR2);
        exit(0);
    }

    key_t key = ftok(MESSAGE_QUEUE, 'A');
    if(key == -1){
        perror("ftok");
        kill(getppid(), SIGUSR2);
        exit(0);
    }

    int msg_queue_id = msgget(key, IPC_CREAT | 0600);
    if(msg_queue_id == -1){
        perror("msgget");
        kill(getppid(), SIGUSR2);
        exit(0);
    }
    
    int my_id = getppid(); // Mobile user id (parent process id)
    ssize_t bytes_read = msgrcv(msg_queue_id, &received_message, sizeof(received_message) - sizeof(long), my_id, IPC_NOWAIT);
    if(bytes_read == -1){
        if(errno == ENOMSG){
            return NULL;
        } else{
            perror("msgrcv");
            return "-1";
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

int can_make_request(time_t last_request_time, int interval){
    time_t current_time = time(NULL);
    return (current_time - last_request_time >= interval);
}

int generate_random_number(int min, int max){
    return rand() % (max - min + 1) + min;
}

void clean_resources(){
    printf("Cleaning resources...\n");
    // Close user_pipe
    close(fd_user_pipe);

    // Free message
    if(message != NULL) free(message);
        
    printf("Resources cleaned. Exiting...\n");
    kill(child_pid, SIGKILL);
}

void handle_signal(int signal){
    if(signal == SIGINT){
        printf("\nCTRL+C pressed. ");
        clean_resources();
        exit(0);
    }

    if(signal == SIGUSR1){
        logged = 1;
    }

    if(signal == SIGUSR2){
        clean_resources();
        exit(0);
    }
}

// mobile.c