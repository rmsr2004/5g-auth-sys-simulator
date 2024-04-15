// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef SM_LIB_H
#define SM_LIB_H

/*
* Responsible for handling the functions of the Authorization Requests Manager process.
*/
void auth_request_manager();
/*
* Responsible for handling the functions of the Monitor Engine process.
*/
void monitor_engine();
/*
* Responsible for handling the functions of the Receiver thread.
*/
void* receiver(void* id);
/*
* Responsible for handling the functions of the Sender thread.
*/
void* sender(void* id);
/*
* Reads from the user_pipe.
*/
char* read_from_user_pipe();
/*
* Reads from the back_pipe.
*/
char* read_from_back_pipe();
/*
* Responsible for handling the SIGINT signal.
*/
void handle_sigint();
/*
* Returns the maximum between a and b.
*/
int max(int a, int b);

#endif