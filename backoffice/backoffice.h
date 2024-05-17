// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef BACKOFFICE_H
#define BACKOFFICE_H

/*
*   Read messages from message queue.
*/
void child_process();
/*
*   Writes messsage on back pipe.
*/
int write_to_back_pipe(char* message);
/*
*   Reads message from message queue.
*/
char* read_message_queue();
/*
*   Cleans up resources.
*/
void cleanup_resources();
/*
*   Signal handler for SIGINT.
*/
void signal_handler(int signal);

#endif // BACKOFFICE_H