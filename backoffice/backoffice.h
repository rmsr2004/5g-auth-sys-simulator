#ifndef BACKOFFICE_H
#define BACKOFFICE_H

/*
* Writes messsage on back pipe.
*/
int write_to_back_pipe(char* message);
/*
* Reads message from message queue.
*/
char* read_message_queue();
/*
* Cleans up resources.
*/
void cleanup_resources();
/*
* Signal handler for SIGINT.
*/
void signal_handler(int signo);

#endif // BACKOFFICE_H