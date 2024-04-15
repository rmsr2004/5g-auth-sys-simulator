#include <fcntl.h>

#ifndef GLOBALS_H
#define GLOBALS_H

#define USER_PIPE       "user_pipe"
#define BACK_PIPE       "back_pipe"
#define MESSAGE_SIZE    100
#define INPUT_SIZE      50

extern int fd_user_pipe, fd_back_pipe;
extern pid_t auth_pid, monitor_pid;
extern int MOBILE_USERS,
           QUEUE_POS,
           AUTH_SERVERS,
           AUTH_PROC_TIME,
           MAX_VIDEO_WAIT,
           MAX_OTHERS_WAIT;

/*
* Checks if a string is a number.
*/
int is_number(char* str);
/*
* Removes '\\n' from the string.
*/
void remove_line_break(char* string);

#endif
