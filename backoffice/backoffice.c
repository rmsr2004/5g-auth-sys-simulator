// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "backoffice.h"
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <wait.h>
#include <semaphore.h>

/*********************************************************************************************************
*                                          BACKOFFICE USER                                               *                                           
**********************************************************************************************************
*   Process that manages aggregated information on user plafonds. It receives periodic statistics        *
*   (produced by the Monitor Engine) via the Message Queue. It can also proactively request statistics   *
*   using a specific command. In this case, the command is sent to the Authorisation Request Manager via * 
*   the BACK_PIPE named pipe, and stored in the Other_Services_Queue queue. The Authorisation Engine is  *
*   responsible for processing the statistics and sending to the BackOffice User via the Message Queue.  *
*                                                                                                        *
*   Syntax of the BackOffice User process initialisation command:                                        *  
*       $ backoffice_user                                                                                *
*                                                                                                        *                                         
*   Information to send to the named pipe:                                                               *                     
*       ID_backoffice_user#[data_stats | reset]                                                          *
*                                                                                                        *
*   The identifier of the BackOffice User to be used is 1.                                               *                            
*   This process receives the following commands from the user:                                          *                       
*       - data_stats - displays statistics regarding data consumption in the various services:           *
*                      total data reserved and number of authorisation renewal requests;                 *
*       - reset - clears the related statistics calculated so far by the system.                         *
*                                                                                                        *                                   
*   The process ends when it receives a SIGINT signal, or in the event of an error. An error can occur   *
*   if a parameter is wrong or if you try to write to the named pipe and the write fails, in which case  *
*   you should write the error message on the screen. Whenever it terminates, the process should clear   *
*   all resources.                                                                                       *                                        
*********************************************************************************************************/

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                          Global Variables                                                             */
/* ----------------------------------------------------------------------------------------------------------------------*/
char* message = NULL;       // Variable to store messages.
char* input = NULL;         // Variable to store user input.
char* token = NULL;         // Variable to store tokens.
pid_t child_pid;            // Variable to store child process PID.
sem_t* sem;                 // Semaphore to ensure that only one process is running.
/* ----------------------------------------------------------------------------------------------------------------------*/

int main(int argc, char* argv[]){
    signal(SIGINT, signal_handler); // CTRL+C signal handler
    signal(SIGUSR2, signal_handler); // SIGUSR2 signal handler

    /*
    *   Check if the program was run with the correct number of arguments
    */
    if(argc != 1){
        fprintf(stderr, "Invalid run\nUsage: %s\n", argv[0]);
        return -1;
    }
        
    /*
    *  Create semaphore to ensure that only one process is running
    */   
    sem = sem_open("sem", O_CREAT | O_EXCL, 0700, 1);
    if(sem == SEM_FAILED){
        if(errno == EEXIST){
            fprintf(stderr, "Another instance of the program is already running.\n");
            return -1;
        } else{
            perror("sem_open");
            return -1;
        }
    }

    input = (char*) malloc(sizeof(char) * INPUT_SIZE);
    if(input == NULL){
        perror("malloc(): ");
        return -1;
    }

    /*
    *   Read message from message queue
    */

    message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        perror("malloc(): ");
        return -1;
    }
    
    // Create process to read messages from message queue
    if((child_pid = fork()) == 0){
        child_process();
        exit(0);
    }
    
    /*
    *   Command Line Interface
    */

    while(1){
        if(fgets(input, INPUT_SIZE, stdin) == NULL){
            perror("fgets()");
            return -1;
        }

        char input_copy[strlen(input) + 1];
        strcpy(input_copy, input);

        token = strtok(input, "#");
        if(atoi(token) != 1){
            fprintf(stderr, "ID must be 1\n");
            break;
        }

        token = strtok(NULL, "\n");
        if (strcmp(token, "data_stats") != 0 && strcmp(token, "reset") != 0) {
            fprintf(stderr, "Invalid command: ID_backoffice_user#[data_stats | reset]\n");
            break;
        }
        
        if(write_to_back_pipe(input_copy) == -1){
            break;
        }
    }
    cleanup_resources();
    return 0;
}

void child_process(){
    signal(SIGINT, SIG_DFL); // CTRL+C signal handler
    while(1){
        // Read message from message queue
        char* msg_from_queue = read_message_queue();
        if(msg_from_queue != NULL){
            if(strcmp(msg_from_queue, "-1") == 0){
                kill(getppid(), SIGUSR2);
            }
            printf("%s\n", msg_from_queue);
            free(msg_from_queue);
        }
    }
}

int write_to_back_pipe(char* message){
    remove_line_break(message);

    int fd_back_pipe = open(BACK_PIPE, O_WRONLY);
    if(fd_back_pipe == -1){
        perror("open(): ");
        return -1;
    }

    ssize_t bytes_written = write(fd_back_pipe, message, strlen(message) + 1);
    if(bytes_written == -1){
        perror("write(): ");
        return -1;
    }
    
    close(fd_back_pipe);
    return 1;
}

char* read_message_queue() {
    message_q received_message;

    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);

    key_t key = ftok(MESSAGE_QUEUE, 'A');
    if(key == -1){
        perror("ftok(): ");
        return "-1";
    }

    int msg_queue_id = msgget(key, IPC_CREAT | 0600);
    if(msg_queue_id == -1){
        perror("msgget(): ");
        return "-1";
    }

    ssize_t bytes_read = msgrcv(msg_queue_id, &received_message, sizeof(received_message) - sizeof(long), 1, IPC_NOWAIT);
    if(bytes_read == -1){
        if(errno == ENOMSG){
            return NULL;
        } else{
            perror("msgrcv(): ");
            return "-1";
        }
    }

    received_message.message[bytes_read] = '\0';
    strcpy(message, received_message.message);
    return message;
}

void cleanup_resources(){
    if(message != NULL) free(message);
    if(input != NULL) free(input);

    sem_close(sem);
    sem_unlink("sem");
}

void signal_handler(int signal){
    if(signal == SIGINT){
        printf("\nCTRL+C pressed. Exiting...\n");
        cleanup_resources();
        kill(child_pid, SIGKILL);
        exit(0);
    }

    if(signal == SIGUSR2){
        cleanup_resources();
        kill(child_pid, SIGKILL);
        exit(0);
    }
} 

// backoffice.c