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
#include <sys/select.h>

char* message = NULL;
char* input = NULL;
char* token = NULL;

int main(int argc, char* argv[]){
    signal(SIGINT, signal_handler); // CTRL+C signal handler

    // Check if the program was run with the correct number of arguments
    if(argc != 1){
        fprintf(stderr, "Invalid run\nUsage: %s\n", argv[0]);
        return -1;
    }

    input = (char*) malloc(sizeof(char) * INPUT_SIZE);
    if(input == NULL){
        perror("malloc(): ");
        return -1;
    }

    // Read message from message queue
    message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        perror("malloc(): ");
        return -1;
    }

    fd_set fds;
    struct timeval timeout;

    while(1){
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        // Setting timeout to 0 to poll for message without blocking
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        // Checking for input from stdin
        if(select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout) > 0){
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

        // Read message from message queue
        message = read_message_queue();
        if(message != NULL){
            if(strcmp(message, "-1") == 0){
                break;
            }
            printf("Received message from message queue:\n%s\n", message);
            free(message);
            message = NULL;
        }
    }
    cleanup_resources();
    return 0;
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

    printf("Sent to back pipe: %s\n", message);
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

    int msg_queue_id = msgget(key, IPC_CREAT | 0700);
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
    if(token != NULL) free(token);
}

// Signal handler
void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nCTRL+C pressed. Exiting...\n");
        cleanup_resources();
        exit(0);
    }
}