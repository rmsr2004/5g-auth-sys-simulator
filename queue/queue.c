// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#include "queue.h"
#include "../globals/globals.h"
#include "../log/log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

queue_t* create_queue(int size){
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    queue->head = NULL;
    queue->tail = NULL;
    queue->max_size = size;
    queue->size = 0;

    return queue;
}

int is_empty(queue_t* queue){
    return queue->head == NULL;
}

int is_full(queue_t* queue){
    return queue->size == queue->max_size;
}

int is_queue_half_full(queue_t* queue){
    int half_max = queue->max_size / 2;
    if (queue->max_size % 2 != 0) {
        half_max++;
    }
    return queue->size <= half_max;
}

void destroy_queue(queue_t* queue){
    if(queue == NULL){
        return;
    }

    queue_node* temp;
    while(!is_empty(queue)){
        if(queue->head == NULL){
            break;
        }
        temp = queue->head;
        queue->head = queue->head->next;
        free(temp);
    }

    if(queue->head == NULL)
        queue->tail = NULL;

    free(queue);
    return;
}

int insert(queue_t* queue, struct request_t new_request){
    if(queue->size >= queue->max_size){
        return 0;
    }

    queue_node* new_node = (queue_node*) malloc(sizeof(queue_node));
    if(new_node == NULL){
        fprintf(stderr, "Error malloc()\n");
        return 0;
    }

    new_node->request = new_request;
    new_node->next = NULL;

    if(is_empty(queue))
        queue->head = new_node;
    else
        queue->tail->next = new_node;

    queue->tail = new_node;
    queue->size++;
    return 1;
}

struct request_t drop(queue_t* queue){
    if(is_empty(queue)){
        return (struct request_t) {0, "0", 0, {0, 0}};
    }

    struct timeval current_time;
    gettimeofday(&current_time, NULL); // Obter o tempo atual

    while(queue->head != NULL){
        if(strcmp(queue->head->request.request_type, "REGISTER") == 0){
            break;
        }
        
        long elapsed_time = (current_time.tv_sec - queue->head->request.creation_time.tv_sec) * 1000 + 
                            (current_time.tv_usec - queue->head->request.creation_time.tv_usec) / 1000;
        
        if(strcmp(queue->head->request.request_type, "VIDEO") == 0 && elapsed_time >= MAX_VIDEO_WAIT){
            update_log("SENDER: %s REQUEST (ID = %d) DROPPED BECAUSE IT WASN'T PROCESSED IN TIME", queue->head->request.request_type, 
                                                                                                   queue->head->request.mobile_id);
            remove_last_node(queue);
        } else if(strcmp(queue->head->request.request_type, "VIDEO") != 0 && elapsed_time >= MAX_OTHERS_WAIT){
            update_log("SENDER: %s REQUEST (ID = %d) DROPPED BECAUSE IT WASN'T PROCESSED IN TIME", queue->head->request.request_type, 
                                                                                                   queue->head->request.mobile_id);
            remove_last_node(queue);
        } else{
            break;
        }
    }

    if(is_empty(queue)){
        return (struct request_t) {0, "0", 0, {0, 0}};
    }

    struct request_t request = queue->head->request;
    queue_node* temp = queue->head;
    queue->head = queue->head->next;
    if(queue->head == NULL){
        queue->tail = NULL;
    }
    queue->size--;
    free(temp);

    return request;
}

int remove_last_node(queue_t* queue){
    if(is_empty(queue)){
        return -1;
    }

    queue_node* temp = queue->head;
    queue_node* prev = NULL;

    while(temp->next != NULL){
        prev = temp;
        temp = temp->next;
    }

    if(prev != NULL){
        prev->next = NULL;
        queue->tail = prev;
        free(temp);
    } else{
        free(queue->head);
        queue->head = NULL;
        queue->tail = NULL;
    }
    queue->size--;
    return 1;
}

void print_queue(queue_t* queue){
    if(is_empty(queue)){
        fprintf(stderr, "Queue is empty\n");
        return;
    }

    queue_node* temp = queue->head;
    while(temp != NULL){
        printf("%d-%s-%d\n", temp->request.mobile_id, temp->request.request_type, temp->request.request_data);
        temp = temp->next;
    }
    return;
}