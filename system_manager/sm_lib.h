// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef SM__LIB_H
#define SM__LIB_H

#include "../shared_memory/shm_lib.h"
#include <sys/time.h>

/*-----------------------------------------------------------------------------------*/
/*                  AUTHORIZATION REQUESTS MANAGER                                   */
/*-----------------------------------------------------------------------------------*/

/*
* Responsible for handling the functions of the Authorisation Requests Manager process:
*   - Creates the named pipes USER_PIPE and BACK_PIPE
*   - Creates unnamed pipes for each Authorisation Engine
*   - Creates the Receiver and Sender threads
*   - Creates the internal data structures: Video_Streaming_Queue and Others_Services_Queue
*   - Creation and removal of Authorisation Engine processes according to the occupancy rate of the
      of the queues.
*/
void auth_request_manager();

/*-----------------------------------------------------------------------------------*/
/*                      MONITOR ENGINE and its functions                             */
/*-----------------------------------------------------------------------------------*/

/*
*   This process checks when each Mobile User's data allowance reaches 80 per cent, 90 per cent or 
*   100 per cent of the initial allowance. As a result, alerts are generated for the respective Mobile Users 
*   via the Message Queue. In addition to the alerts, this process is also responsible for generating 
*   periodic statistics (at 30-second intervals) for the BackOffice User.
*/
void monitor_engine();
/*
*   Send system data to the backoffice.
*/
void send_data_to_backoffice(struct shm_system_data* system_data);
/*
*   Send data periodically (30-second intervals) to the backoffice.
*/
void send_data_periodically();
/*
*   Send plafond alerts to a mobile phone when he reachs 100%, 90% or 80% usage.
*/
void send_plafond_alerts();
/*
*   Round the number to 0, 10 or 20 if number is close to 0, 10 or 20. 
*/
void round_number(int* number);

/*-----------------------------------------------------------------------------------*/
/*                      RECEIVER THREAD and its functions                            */
/*-----------------------------------------------------------------------------------*/

/*
*   Responsible for handling the functions of the Receiver thread:
*       - Reads from user_pipe and back_pipe.
*       - Place requests in the Video_Streaming_Queue and Others_Services_Queue.
*/
void* receiver();
/*
*   Processes the message received from the user_pipe.
*   Returns 0 if is a request message, 1 if is a register message.
*/
int process_msg_from_user_pipe(char* message, char* array[]);
/*
*   Processes the message received from the back_pipe.
*   Return 1 if is a request message, 0 if error.
*/
int process_msg_from_back_pipe(char* message, char* array[]);
/*
*   Reads from the user_pipe and returns the message.
*/
char* read_from_user_pipe();
/*
*   Reads from the back_pipe and returns the message.
*/
char* read_from_back_pipe();

/*-----------------------------------------------------------------------------------*/
/*                              SENDER THREAD                                        */
/*-----------------------------------------------------------------------------------*/

/*
*   Responsible for handling the functions of the Sender thread:
*       - This thread is notified when a new authorisation request is placed in one of the queues.
*         At that point, it will have to select an Authorisation Engine that is available and
*         submit the authorisation request for execution.
*       - When one of the queues is full, an extra Authorisation Engine must be created. Removing
*         the extra Authorisation Engine should be removed when the queue reaches 50%.
*       - The initial number of Authorisation Engines is given by AUTH_SERVERS, this parameter being
*         provided by the configuration file.
*       - The processing time for each Authorisation Engine to process the requested task
*         is given by AUTH_PROC_TIME;
*       - Video authorisation requests have MAX_VIDEO_WAIT to be fulfilled. Authorisation
*         authorisation requests placed in the lowest priority queue have MAX_OTHERS_WAIT to be
*         answered. If the request is not answered within the time limit, it is deleted by the
*         Sender thread and this information is written to the screen and the log file.
*       - Communication between the Sender and the Authorisation Engine takes place via the unnamed pipe
*         created for each Authorisation Engine. When the Authorisation Engine is processing
*         an authorisation request, it must be in the busy state (so that the Sender doesn't send it another
*         not send it another message until it has been processed).
*/
void* sender();

/*-----------------------------------------------------------------------------------*/
/*                     AUTHORIZATION ENGINE and its functions                        */
/*-----------------------------------------------------------------------------------*/

/*
*   Responsible for handling the functions of the Authorisation Engine process:
*       - Initial registration: initial message to simulate the Mobile User's registration on 
*         the service authorisations platform. The Mobile User's initial plafond is indicated in 
*         this message and is recorded in Shared Memory.
*       - Authorisation request: sent at the start of the use of each service, and at periodic intervals, 
*         it contains the amount of data to be reserved for the service. The Authorisation Engineer must 
*         check whether the Mobile User still has enough available in the Shared Memory. If so, subtract the 
*         amount of the reservation from the available ceiling. If they don't have enough, the authorisation 
*         request must be rejected and this information written on the screen and in the log file. To calculate the
*         statistics, the total amount of data requested for each type of service must be kept, as well as the total
*         number of requests that have been received for each type of service.
*       - Statistics request: command sent by the BackOffice user requesting statistics. The Authorisation Engine must 
*         access the Shared Memory, read the necessary data and return it to the BackOffice User via the Message Queue.
*       - Statistics reset: command sent by the BackOffice user requesting a statistics reset. The Authorisation Engine 
*         must reset the statistics in the Shared Memory.
*/
void authorization_engine(int id);
/*
* Wait for the processing time to finish.
*/
void wait_for_processing_time(struct timeval start_time);
/*
* Change 'volatile sig_atomic_t sm_is_closed' to 1 to signal the system manager to close.
*/
void sigusr1_handler(int signal);

/*-----------------------------------------------------------------------------------*/
/*                            SYSTEM MANAGER FUNCTIONS                               */
/*-----------------------------------------------------------------------------------*/

/*
* Creates message queue.
*/
void create_message_queue();
/*
* Removes message queue.
*/
void remove_message_queue();
/*
* Cleanup resources:
* - Close the message queue.
* - Close unnamed pipes.
* - Close named pipes.
* - Destroy queues.
* - Destroy unnamed pipes array.
* - Destroy authorization engines array.
* - Close the shared memory.
*/
void cleanup_resources();
/*
* Responsible for handling the signalINT signalnal.
*/
void handle_signal(int signal);

#endif // SM__LIB_H