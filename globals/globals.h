// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef GLOBALS_H
#define GLOBALS_H

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                             Definitions                                                               */
/* ----------------------------------------------------------------------------------------------------------------------*/
#define USER_PIPE           "/tmp/user_pipe"                // Path to the user pipe.
#define BACK_PIPE           "/tmp/back_pipe"                // Path to the back pipe.
#define MESSAGE_QUEUE       "/tmp"                          // Path to the message queue.
#define SEM_MSG_QUEUE       "sem_msg_queue"                 // Name of the semaphore for the message queue.
#define SHM_SEM             "shm_sem"                       // Name of the semaphore for the shared memory.
#define MESSAGE_SIZE        100                             // Size of the message in the message queue.
#define INPUT_SIZE          50                              // Size of the input buffer.

// Struct representing a message in the message queue.
typedef struct{
    long type;
    char message[MESSAGE_SIZE];
} message_q;
/* ----------------------------------------------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                          Global Variables                                                             */
/* ----------------------------------------------------------------------------------------------------------------------*/
extern pid_t auth_pid;                                      // PID of the Authentication Requests Manager server.
extern pid_t monitor_pid;                                   // PID of the Monitor Engine process.
extern int MOBILE_USERS;                                    // Number of mobile users.
extern int QUEUE_POS;                                       // Max number of requestes in the queue.
extern int AUTH_SERVERS;                                    // Number of authentication servers.
extern int AUTH_PROC_TIME;                                  // Time to process an authentication request.
extern int MAX_VIDEO_WAIT;                                  // Maximum time to wait for a video.
extern int MAX_OTHERS_WAIT;                                 // Maximum time to wait for other requests.
extern int shmid;                                           // Shared Memory ID.
extern sem_t* shm_sem;                                      // Semaphore for shared memory.
extern void* shared_var;                                    // Shared memory variable.
/* ----------------------------------------------------------------------------------------------------------------------*/
  

/* ----------------------------------------------------------------------------------------------------------------------*/
/*                                              Functions                                                                */
/* ----------------------------------------------------------------------------------------------------------------------*/

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