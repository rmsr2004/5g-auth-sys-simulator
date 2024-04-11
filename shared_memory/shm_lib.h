// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef SHM_LIB_H
#define SHM_LIB_H

#define SHM_SIZE 1024

// Create Shared Memory.
int create_shared_memory();
// Attach shared memory.
int* attach_shared_memory(int shmid);
// Detach Shared Memory.
int detach_shared_memory(int* shared_var);
// Remove Shared Memory.
void remove_shared_memory(int shmid);

#endif