#ifndef GLOBALS_H
#define GLOBALS_H

#include <fcntl.h>
#include <pthread.h>

/* 
*   Definitions 
*/
#define USER_PIPE       "/tmp/user_pipe"        // Path to the user pipe.
#define BACK_PIPE       "/tmp/back_pipe"        // Path to the back pipe.
#define MESSAGE_QUEUE   "/tmp"                  // Path to the message queue.
#define MESSAGE_SIZE    100                     // Size of the message in the message queue.
#define INPUT_SIZE      50                      // Size of the input buffer.                                          

/* 
*   Global Variables 
*/
extern pid_t auth_pid;                                      // PID of the Authentication Requests Manager server.
extern pid_t monitor_pid;                                   // PID of the Monitor Engine process.
extern void* shared_var;                                    // Shared Memory pointer.
extern int MOBILE_USERS;                                    // Number of mobile users.
extern int QUEUE_POS;                                       // Max number of requestes in the queue.
extern int AUTH_SERVERS;                                    // Number of authentication servers.
extern int AUTH_PROC_TIME;                                  // Time to process an authentication request.
extern int MAX_VIDEO_WAIT;                                  // Maximum time to wait for a video.
extern int MAX_OTHERS_WAIT;                                 // Maximum time to wait for other requests.
extern int fd_user_pipe;                                    // File descriptor for the user pipe.
extern int fd_back_pipe;                                    // File descriptor for the back pipe.
extern int shmid;                                           // Shared Memory ID.
extern int msg_queue_id;                                    // Message Queue ID.
  
// Struct representing a message in the message queue.
typedef struct{
    long type;
    char message[MESSAGE_SIZE];
} message_q;

/*
* Function Prototypes.
*/

/*
* Checks if a string is a number.
*/
int is_number(char* str);
/*
* Removes '\\n' from the string.
*/
void remove_line_break(char* string);
/*
* Returns the maximum between a and b.
*/
int max(int a, int b);

#endif // GLOBALS_H