// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef SM__LIB_H
#define SM__LIB_H

#include "../queue/queue.h"

/*
* Responsible for handling the functions of the Authorization Requests Manager process:
* - Creates unnamed pipes.
* - Creates user and back pipes (named pipes).
* - Creates the Receiver and Sender threads.
*/
void auth_request_manager();
/*
* Responsible for handling the functions of the Monitor Engine process.
*/
void monitor_engine();
/*
* Send system data to the backoffice.
*/
void send_data_to_backoffice();
/*
* Send system data to the backoffice periodically in <interval> seconds.
*/
void send_data_periodically(int interval);
/*
* Send plafond alerts to a mobile phone when he reachs 100%, 90% or 80% usage.
*/
void send_plafond_alerts();
/*
* Responsible for handling the functions of the Receiver thread:
* - Reads from user_pipe and back_pipe.
*/
void* receiver();
/*
* Processes the message received from the user_pipe.
* Returns 0 if is a request message, 1 if is a register message.
*/
int process_msg_from_user_pipe(char* message, char* array[]);
/*
* Processes the message received from the back_pipe.
*/
int process_msg_from_back_pipe(char* message, char* array[]);
/*
* Reads from the user_pipe.
*/
char* read_from_user_pipe();
/*
* Reads from the back_pipe.
*/
char* read_from_back_pipe();
/*
* Responsible for handling the functions of the Sender thread.
*/
void* sender();
/*
* Responsible for handling the functions of the Authorization Engine process.
*/
void authorization_engine(int id);
/*
* Returns the id of the Authorization Engine process available.
* If there is no Authorization Engine process available, returns 0.
*/
int is_engine_available();
/*
* Creates a message queue.
*/
void create_message_queue();
/*
* Removes a message queue.
*/
void remove_message_queue();
/*
* Responsible for handling the SIGINT signal.
*/
void handle_sigint();


#endif // SM__IB_H