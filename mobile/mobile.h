#ifndef MOBILE_H
#define MOBILE_H

// Global variables to control the time of the last request of each type
extern time_t last_social_request;
extern time_t last_music_request;
extern time_t last_video_request;

extern char* message;

/*
* Function to handle the SIGINT signal.
*/
void handle_sigint();
/*
* Function to clean the resources used by the mobile.
*/
void clean_resources();
/*
* Write the message to user_pipe.
*/
void write_to_user_pipe(char* message);
/*
* Verify if is possible to make a new request, according to the last request time and the interval.
*/
int can_make_request(time_t last_request_time, int interval);
/*
* Read the message from the message queue.
*/
char* read_message_queue();
/*
* Process the message from the message queue.
*/
int process_message_from_queue(char* message);
/*
* Function to generate a random number between min and max.
*/
int generate_random_number(int min, int max);


#endif