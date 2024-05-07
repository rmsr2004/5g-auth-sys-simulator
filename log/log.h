// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef LOG_H
#define LOG_H

#include <semaphore.h>

// Global variable to save log_file once he is created.
extern char* log_file;

// Semaphore to avoid two process writing at same time.
extern sem_t* sem_log;


/*
* Creates log file.
* @param file Destination and file name.
*/
void create_log_file(char* file);
/*
* Updates log file adding the new action at the end of the text.
* @param action Log action
*/
void update_log(const char* action, ...);
/*
* Close log functions and log_file is released and the semaphore is closed.
*/
void close_log();

#endif