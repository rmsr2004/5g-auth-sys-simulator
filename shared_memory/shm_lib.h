// João Afonso dos Santos Simões - 2022236316
// Rodrigo Miguel Santos Rodrigues - 2022233032
#ifndef SHM_LIB_H
#define SHM_LIB_H

#define KEY_FILE    "/tmp"      // Path to create key file.
#define MAX_SIZE    100         // Max size of array.

// Struct to store mobile shared memory data.
struct shm_mobile_data{
    int mobile_id;
    int initial_plafond;
    int current_plafond;
    int alert_sent[3];
};

// Struct to store system shared memory data.
struct shm_system_data{
    int engines_available[MAX_SIZE];
    int current_mobile_users;
    int video_plafond_used;
    int social_plafond_used;
    int music_plafond_used;
    int video_requests;
    int social_requests;
    int music_requests;
};

// Create Shared Memory.
int create_shared_memory(int size);
// Attach shared memory.
void* attach_shared_memory(int shmid);
// Detach Shared Memory.
int detach_shared_memory(void* shared_var);
// Remove Shared Memory.
void remove_shared_memory(int shmid);
// Print Shared Memory.
void print_memory(struct shm_mobile_data* mobile_data, struct shm_system_data* system_data);

#endif // SHM_LIB_H