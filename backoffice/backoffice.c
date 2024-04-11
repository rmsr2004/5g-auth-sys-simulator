// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define INPUT_SIZE 50

// Semaphore to ensure that only one process is running.
sem_t* sem;

/*
* Removes '\\n' from the string.
*/
void remove_line_break(char* string);

int main(int argc, char* argv[]){
    /*
    *   Checks if Backoffice has been run correctly
    */
    if(argc != 1){
        fprintf(stderr, "Invalide Run\nUsage: %s \n", argv[0]);
        return -1;
    }
    
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
    
    /*
    * User Input
    */
    char* input = (char*) malloc(sizeof(char) * INPUT_SIZE);
    if(input == NULL){
        fprintf(stderr, "Error malloc()\n");
        sem_close(sem);
        sem_unlink("sem");
        return -1;
    }

    if(fgets(input, INPUT_SIZE, stdin) == NULL){
        fprintf(stderr, "fgets()\n");
        sem_close(sem);
        sem_unlink("sem");  
        return -1;
    }
    remove_line_break(input);

    /*
    * Handle input.
    */
    char* token = (char*) malloc(sizeof(char) * INPUT_SIZE);
    if(token == NULL){
        fprintf(stderr, "token\n");
        sem_close(sem);
        sem_unlink("sem");
        return -1;
    }

    strcpy(token, strtok(input, "#"));
    if(atoi(token) != 1){
        fprintf(stderr, "ID must be 1\n");
        sem_close(sem);
        sem_unlink("sem");
        return -1;
    }

    strcpy(token, strtok(NULL, ""));
    if(strcmp(token, "data_stats") != 0 && strcmp(token, "reset") != 0){
        fprintf(stderr, "Invalid command: ID_backoffice_user#[data_stats | reset]\n");
        sem_close(sem);
        sem_unlink("sem");
        return -1;
    }
    
    printf("Valid command!\n");

    sem_close(sem);
    sem_unlink("sem");
    return 0;
}

void remove_line_break(char* string){
    for(size_t i = 0; i < strlen(string); i++){
        if(string[i] == '\n')
            string[i] = '\0';
    }
    return;
}