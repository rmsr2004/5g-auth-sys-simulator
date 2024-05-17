// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef QUEUE_H
#define QUEUE_H

#include <sys/time.h>

// Struct containing the request information.
struct request_t{
    int mobile_id; // Mobile ID.
    char request_type[10]; // Request type.
    int request_data; // Request data.
    struct timeval creation_time; // Request creation time.
};

typedef struct queue_node{
    struct request_t request;
    struct queue_node* next;
} queue_node;

typedef struct{
    queue_node* head;
    queue_node* tail;
    int max_size; // Maximum size of the queue.
    int size; // Current size of the queue.
} queue_t;

/*
*   Returns a pointer to a new queue with size.
*/
queue_t* create_queue(int size);
/*
*   Returns 1 if the queue is empty, 0 otherwise.
*/
int is_empty(queue_t* queue);
/*
*   Returns 1 if the queue is full, 0 otherwise.
*/
int is_full(queue_t* queue);
/*
*   Returns 1 if the queue is half full, 0 otherwise.
*/
int is_queue_half_full(queue_t* queue);
/*
*   Frees the memory allocated for the queue.
*/
void destroy_queue(queue_t* queue);
/*
*   Inserts a new request in the queue.
*/
int insert(queue_t* queue, struct request_t new_request);
/*
*   Removes the first request from the queue and returns it.
*/
struct request_t drop(queue_t* queue);
/*
*   Removes the first request from the queue and returns 1 if successful, 0 otherwise.
*   It also removes all requests that are waiting longer than the limit set by the System Manager.
*/
int remove_last_node(queue_t* queue);
/*
*   Prints the queue.
*/
void print_queue(queue_t* queue);

#endif // QUEUE_H