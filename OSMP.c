//
// Created by Fabian, Moritz on 17.04.18.
//

#include "OSMP.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#ifdef __APPLE__

#include <zconf.h>

#endif
#ifdef __linux__
#include <unistd.h>
#endif

#include <string.h>
#include <errno.h>
#include <math.h>
#include "int_to_string.h"

static sem_t* read_semaphore = NULL, * write_semaphore = NULL, * messages_semaphore = NULL;

struct stat shared_memory_file_stat;

static SHARED_MEMORY* shared_memory = NULL;

static unsigned int current_rank;

static sem_t** messages_semaphores = NULL;
static char** messages_name_semaphores = NULL;

static sem_t** messages_limit_semaphores = NULL;
static char** messages_name_limit_semaphores = NULL;

/**
 * Adds the message index to the given index storage
 * @param first_last the index storage
 * @param index the index of the message to add
 */
void add_message(FIRST_LAST* first_last, int index) {
    if (first_last->first == -1) { // First message
        first_last->first = index; // On first message, last and first is identically
        first_last->last = index;
    } else { // There's already an first message
        shared_memory->messages[first_last->last].next = index; // Add the next message as an next of the last message
        first_last->last = index; // Message is now the last message
    }
}

/**
 * Removes the message index from the given index storage
 * @param first_last the index storage
 * @param index the index of the message to remove
 */
void remove_message(FIRST_LAST* first_last, int index) {
    // Declare next free index from message previous next free message
    first_last->first = shared_memory->messages[index].next;
    if (first_last->first == -1) { // When there is not first message, there is no last message
        first_last->last = -1;
    }
}

/**
 * Resets an message to bring it back to it's initial state
 * @param message the message to reset
 */
void reset_message(MESSAGE* message) {
    message->next = -1;
}

/**
 * Fill the given message with the data, size, source and destination
 * @param message the message to send
 * @param buffer the data of the message
 * @param size the size of the data
 * @param source the source process index of the message
 * @param destination the destination of the message
 */
void fill_message(MESSAGE* message, const void* buffer, size_t size, unsigned int source, unsigned int destination) {
    memcpy(message->data, buffer, size);
    message->length = size;
    message->source = source;
    message->destination = destination;
    reset_message(message);
}

/**
 * Reads the given message and pass back the data, size, source and length
 * @param message message to read
 * @param buffer the buffer of the message
 * @param size the size  of the message
 * @param source the source process index of the message
 * @param len the length of the message
 */
void read_message(MESSAGE* message, void* buffer, size_t size, unsigned int* source, size_t* len) {
    memcpy(buffer, message->data, size);
    *source = message->source;
    *len = message->length;
    reset_message(message);
}

/**
 * Checks if the OSMP library got initialized
 * ```
 * int main(int argc, char* argv[]) {
 *  if (OSMP_Init(&argc, &argv) == OSMP_ERROR) return EXIT_FAILURE;
 *  ...
 * }
 * ```
 * @return {@link OSMP_SUCCESS} when initialized else {@link OSMP_ERROR}
 */
int check_init() {
    if (shared_memory == NULL || read_semaphore == NULL || write_semaphore == NULL) {
        printf("OSMP_Init() needs to be called first");
        return OSMP_ERROR;
    }
    return OSMP_SUCCESS;
}

int OSMP_Init(int* argc, char*** argv) {
    printf("%s", "OSMP_Init()\n");

    int fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0);
    if (fd < 1) {
        printf("shm_open(): %s\n", strerror(errno));
        return OSMP_ERROR;
    }

    if (fstat(fd, &shared_memory_file_stat) < 0) {
        printf("fstat(): %s\n", strerror(errno));
        return OSMP_ERROR;
    }

    shared_memory = (SHARED_MEMORY*) mmap(NULL, (size_t) shared_memory_file_stat.st_size,
                                          PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        printf("mmap(): %s\n", strerror(errno));
        return OSMP_ERROR;
    }

    read_semaphore = sem_open(MEMORY_READ_SEMAPHORE_NAME, O_CREAT, 0644, 1);

    write_semaphore = sem_open(MEMORY_WRITE_SEMAPHORE_NAME, O_CREAT, 0644, 1);

    messages_semaphore = sem_open(MESSAGES_SEMAPHORE_NAME, O_CREAT, 0644, OSMP_MAX_SLOTS);

    pid_t pid = getpid();

    unsigned int length = shared_memory->header.size;

    messages_semaphores = malloc(sizeof(sem_t*) * length);

    messages_name_semaphores = malloc(sizeof(char*) * length);

    messages_limit_semaphores = malloc(sizeof(sem_t*) * length);

    messages_name_limit_semaphores = malloc(sizeof(char*) * length);

    for (unsigned int i = 0; i < length; i++) {
        if (shared_memory->header.processes[i].process_id == pid) {
            current_rank = i;
        }
        char* ptr;
        char* sem_rank = intToString(i, &ptr);
        char* sem_name = malloc(sizeof(char) * (strlen(MESSAGES_SEMAPHORE_NAME_PREFIX) + strlen(sem_rank)) + 1);
        strcpy(sem_name, MESSAGES_SEMAPHORE_NAME_PREFIX);
        strcat(sem_name, sem_rank);
        messages_semaphores[i] = sem_open(sem_name, O_CREAT, 0644, 0);
        messages_name_semaphores[i] = sem_name;
        sem_name = malloc(sizeof(char) * (strlen(MESSAGES_SEMAPHORE_LIMIT_NAME_PREFIX) + strlen(sem_rank)) + 1);
        strcpy(sem_name, MESSAGES_SEMAPHORE_LIMIT_NAME_PREFIX);
        strcat(sem_name, sem_rank);
        messages_limit_semaphores[i] = sem_open(sem_name, O_CREAT, 0644, OSMP_MAX_MESSAGES_PROC);
        messages_name_limit_semaphores[i] = sem_name;
        free(ptr);
    }
    return OSMP_SUCCESS;
}

int OSMP_Finalize(void) {
    if (check_init() == OSMP_ERROR) return OSMP_ERROR;
    sem_close(read_semaphore);
    //sem_unlink(MEMORY_READ_SEMAPHORE_NAME);
    sem_close(write_semaphore);
    //sem_unlink(MEMORY_WRITE_SEMAPHORE_NAME);
    for (unsigned int i = 0, length = shared_memory->header.size; i < length; i++) {
        sem_close(messages_semaphores[i]);
        sem_close(messages_limit_semaphores[i]);
        //sem_unlink(messages_name_semaphores[i]);
        free(messages_name_semaphores[i]);
        free(messages_name_limit_semaphores[i]);
    }
    free(messages_semaphores);
    free(messages_limit_semaphores);
    free(messages_name_semaphores);
    free(messages_name_limit_semaphores);

    munmap(shared_memory, (size_t) shared_memory_file_stat.st_size);
    shared_memory = NULL;
    read_semaphore = NULL;
    write_semaphore = NULL;
    messages_semaphores = NULL;
    messages_limit_semaphores = NULL;
    messages_name_semaphores = NULL;
    messages_name_limit_semaphores = NULL;
    return OSMP_SUCCESS;
}

int OSMP_Size(unsigned int* size) {
    if (check_init() == OSMP_ERROR) return OSMP_ERROR;
    *size = shared_memory->header.size;
    return OSMP_SUCCESS;
}

int OSMP_Rank(unsigned int* rank) {
    if (check_init() == OSMP_ERROR) return OSMP_ERROR;
    *rank = current_rank;
    return OSMP_SUCCESS;
}

int OSMP_Send(const void* buffer, size_t size, unsigned int dest) {
    if (size > OSMP_MAX_PAYLOAD_LENGTH) {
        printf("size > OSMP_MAX_PAYLOAD_LENGTH(%d)\n", OSMP_MAX_PAYLOAD_LENGTH);
        return OSMP_ERROR;
    }
    if (check_init() == OSMP_ERROR) return OSMP_ERROR;

    sem_wait(messages_limit_semaphores[dest]);

    sem_wait(messages_semaphore);

    //printf("Send() Start\n");
    sem_wait(write_semaphore); // wait until there's no writers
    sem_wait(read_semaphore); // wait until there's no readers

    // Write message
    int first_free_index = shared_memory->header.first_last_free.first;
    if (first_free_index == -1) {
        printf("No message slot available\n");
        return OSMP_ERROR;
    }

    remove_message(&shared_memory->header.first_last_free, first_free_index);

    fill_message(&shared_memory->messages[first_free_index], buffer, size, current_rank, dest);

    add_message(&shared_memory->header.processes[dest].first_last_msg, first_free_index);

    sem_post(read_semaphore); // signal other readers can run
    sem_post(write_semaphore); // signal other writers can run

    sem_post(messages_semaphores[dest]);
    return OSMP_SUCCESS;
}

int OSMP_Recv(void* buffer, size_t size, unsigned int* source, size_t* len) {
    //printf("Recv() Start\n");
    if (check_init() == OSMP_ERROR) return OSMP_ERROR;

    sem_wait(messages_semaphores[current_rank]);

    sem_wait(write_semaphore); // wait until there's no writers
    sem_post(read_semaphore); // there's one more reader active

    //printf("Recv_sem_wait() Start\n");
    //printf("Recv_sem_wait() End\n");

    // Read message
    int first_msg_index = shared_memory->header.processes[current_rank].first_last_msg.first;
    if (first_msg_index == -1) {
        printf("No message received\n");
        return OSMP_ERROR;
    }

    remove_message(&shared_memory->header.processes[current_rank].first_last_msg, first_msg_index);

    read_message(&shared_memory->messages[first_msg_index], buffer, size, source, len);

    add_message(&shared_memory->header.first_last_free, first_msg_index);

    sem_wait(read_semaphore); // this reader is completed
    sem_post(write_semaphore); // signal other writers can run

    sem_post(messages_semaphore);

    sem_post(messages_limit_semaphores[current_rank]);
    //printf("Recv() End\n");
    return OSMP_SUCCESS;
}

SHARED_MEMORY* OSMP_Get_Shared_Memory() {
    if (check_init() == OSMP_ERROR) return NULL;
    return shared_memory;
}