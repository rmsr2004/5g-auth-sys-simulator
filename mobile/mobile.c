// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    /*
    *   Checks if Mobile has been run correctly
    */
    if(argc != 7){
        fprintf(stderr, "Invalide Run\nUsage: %s {plafond inicial} / {número máximo de pedidos de autorização} /\n {intervalo VIDEO} / {intervalo MUSIC} / {intervalo SOCIAL} / {dados a reservar}\n", argv[0]);
        return -1;
    }

    for(int i = 1; i < argc; i++){
        if(atoi(argv[i]) <= 0){
            fprintf(stderr, "%s must be > 0\n", argv[i]);
            return -1;
        }
    }

    // proteção caracteres estranhos
    
    int INITIAL_PLAFOND   = argv[1],
        MAX_AUTH_REQUESTS = argv[2],
        VIDEO_GAP         = argv[3],
        MUSIC_GAP         = argv[4],
        SOCIAL_GAP        = argv[5],
        MOBILE_DATA_USE   = argv[6];


    printf("Mobile Process Created!\n");
    sleep(2);

    return 0;
}