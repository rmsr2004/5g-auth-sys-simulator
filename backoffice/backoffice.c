// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/*
* Write the message to back_pipe.
*/
void write_to_back_pipe(char* message);

int main(int argc, char* argv[]){
    /*
    *   Checks if Backoffice has been run correctly
    */
    if(argc != 1){
        fprintf(stderr, "Invalide Run\nUsage: %s \n", argv[0]);
        return -1;
    }

    while(1){    
        /*
        * User Input
        */
        char* input = (char*) malloc(sizeof(char) * INPUT_SIZE);
        if(input == NULL){
            fprintf(stderr, "Error malloc()\n");
            return -1;
        }

        if(fgets(input, INPUT_SIZE, stdin) == NULL){
            fprintf(stderr, "fgets()\n");
            return -1;
        }
        remove_line_break(input);

        char input_copy[strlen(input) + 1];
        strcpy(input_copy, input);

        /*
        * Handle input.
        */
        char* token = (char*) malloc(sizeof(char) * INPUT_SIZE);
        if(token == NULL){
            fprintf(stderr, "token\n");
            return -1;
        }

        strcpy(token, strtok(input, "#"));
        if(atoi(token) != 1){
            fprintf(stderr, "ID must be 1\n");
            return -1;
        }

        strcpy(token, strtok(NULL, ""));
        if(strcmp(token, "data_stats") != 0 && strcmp(token, "reset") != 0){
            fprintf(stderr, "Invalid command: ID_backoffice_user#[data_stats | reset]\n");
            return -1;
        }

        free(input);

        write_to_back_pipe(input_copy);
    }
    return 0;
}

void write_to_back_pipe(char* message){
    fd_back_pipe = open(BACK_PIPE, O_WRONLY);
    write(fd_back_pipe, message, strlen(message) + 1);
    close(fd_back_pipe);
}
