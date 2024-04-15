// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "../globals/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>

/*
* Write the message to user_pipe.
*/
void write_to_user_pipe(char* message);

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

    // Create message to user pipe.
    char* message = (char*) malloc(sizeof(char) * MESSAGE_SIZE);
    if(message == NULL){
        fprintf(stderr, "Error malloc()\n");
        return -1;
    }
    sprintf(message, "%d#%d", mobile_id, INITIAL_PLAFOND);
    write_to_user_pipe(message);
    
    return 0;
}

void write_to_user_pipe(char* message){
    fd_user_pipe = open(USER_PIPE, O_WRONLY);
    write(fd_user_pipe, message, strlen(message) + 1);
    close(fd_user_pipe);
    free(message);
}
