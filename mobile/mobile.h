// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef MOBILE_H
#define MOBILE_H

#include <time.h>


/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                              Functions                                                                */
/* ----------------------------------------------------------------------------------------------------------------------*/

/*
*   Responsible for read messages from the message queue and process them.
*/
void child_process();
/*
*   Write the message to user_pipe.
*/
void write_to_user_pipe(char* message);
/*
*   Read the message from the message queue.
*/
char* read_message_queue();
/*
*   Process the message from the message queue. If message is "ALERT: 0" the mobile will be terminated.
*/
int process_message_from_queue(char* message);
/*
*   Verify if is possible to make a new request, according to the last request time and the interval.
*/
int can_make_request(time_t last_request_time, int interval);
/*
*   Function to generate a random number between min and max.
*/
int generate_random_number(int min, int max);
/*
*   Function to clean the resources used by the mobile.
*/
void clean_resources();
/*
*   Function to handle signals.
*/
void handle_signal(int signal);

#endif  // MOBILE_H