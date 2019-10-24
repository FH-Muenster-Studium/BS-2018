//
// Created by Fabian, Moritz on 17.04.18.
//

#ifndef OSMPRUN_OSMP_H
#define OSMPRUN_OSMP_H

#define OSMP_SUCCESS 0
#define OSMP_ERROR (-1)

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// maximale Zahl der Nachrichten pro Prozess
#define OSMP_MAX_MESSAGES_PROC 16
// maximale Anzahl der Nachrichten, die insgesamt vorhanden sein dürfen
#define OSMP_MAX_SLOTS 256
// maximale Länge der Nutzlast einer Nachricht
#define OSMP_MAX_PAYLOAD_LENGTH 128

#define SHARED_MEMORY_NAME "/OSMP_MEMORY"

#define MEMORY_INIT_SEMAPHORE_NAME "/OSMP_MEMORY_INIT_SEMAPHORE"

#define MEMORY_READ_SEMAPHORE_NAME "/OSMP_MEMORY_READ_SEMAPHORE"

#define MEMORY_WRITE_SEMAPHORE_NAME "/OSMP_MEMORY_WRITE_SEMAPHORE"

#define MESSAGES_SEMAPHORE_NAME "/OSMP_MESSAGES_SEMAPHORE"

#define MESSAGES_SEMAPHORE_NAME_PREFIX "/OSMP_MESSAGES_SEMAPHORE_"

#define MESSAGES_SEMAPHORE_LIMIT_NAME_PREFIX "/OSMP_MESSAGES_LIMIT_SEMAPHORE_"

typedef struct {
    pthread_t thread_id;
    sem_t sem;
    // Send / Receive
    void* buffer;
    int buffer_size;
    // Send
    int dest;
    // Recv
    int source;
    int size;
} OSMP_REQUEST;

typedef struct {
    int first;
    int last;
} FIRST_LAST;

typedef struct {
    pid_t process_id;
    FIRST_LAST first_last_msg;
} PROCESS_HEADER;

typedef struct {
    unsigned int size;
    FIRST_LAST first_last_free;
    PROCESS_HEADER processes[];
} HEADER;

typedef struct {
    unsigned int destination;
    unsigned int source;
    int next; //(free/used)
    size_t length;
    char data[OSMP_MAX_PAYLOAD_LENGTH];
} MESSAGE;

typedef struct {
    MESSAGE messages[OSMP_MAX_SLOTS];
    HEADER header;
} SHARED_MEMORY;

/**
 * Initializes the OSMP library for the current process
 * ```
 * int main(int argc, char* argv[]) {
 *  if (OSMP_Init(&argc, &argv) == OSMP_ERROR) return EXIT_FAILURE;
 *  ...
 * }
 * ```
 * @param argc argument count
 * @param argv arguments
 * @return {@link OSMP_SUCCESS} when initialized else {@link OSMP_ERROR}
 */
int OSMP_Init(int* argc, char*** argv);

/**
 * Save the current amount of processes managed by the OSMP library at the current destination
 * @param size the current destination to save
 * @return {@link OSMP_SUCCESS} when successfully else {@link OSMP_ERROR}
 */
int OSMP_Size(unsigned int* size);

/**
 * Save the internal OSMP library index of the current process at the current destination
 * @param rank the current destination to save
 * @return {@link OSMP_SUCCESS} when successfully else {@link OSMP_ERROR}
 */
int OSMP_Rank(unsigned int* rank);

/**
 * Try to send a message to the given process
 * @param buffer the message data
 * @param size the buffer size
 * @param dest the process index that should receive the message
 * @return {@link OSMP_SUCCESS} when send successfully else {@link OSMP_ERROR}
 */
int OSMP_Send(const void* buffer, size_t size, unsigned int dest);

/**
 * Try to receive the first available message for the current process, will block until there is an message to receive
 * @param buffer the buffer to save the message data into
 * @param size the size of the buffer
 * @param source the sender of the message
 * @param len the actual length of the message, can be equal but don't has to be equal with the buffer size
 * @return {@link OSMP_SUCCESS} when received successfully else {@link OSMP_ERROR}
 */
int OSMP_Recv(void* buffer, size_t size, unsigned int* source, size_t* len);

/**
 * Finalizes the OSMP library for the current process
 * ```
 * int main(int argc, char* argv[]) {
 *  ...
 *  if (OSMP_Finalize() == OSMP_ERROR) return EXIT_FAILURE;
 *  return EXIT_SUCCESS;
 * }
 * ```
 * @return {@link OSMP_SUCCESS} when finalized else {@link OSMP_ERROR}
 */
int OSMP_Finalize(void);

int OSMP_Isend(const void* buf, int count, unsigned int dest, OSMP_REQUEST request);

int OSMP_Irecv(void* buf, int count, int* source, int* len,
               OSMP_REQUEST request);

int OSMP_Test(OSMP_REQUEST request, int* flag);

int OSMP_Wait(OSMP_REQUEST request);

int OSMP_CreateRequest(OSMP_REQUEST* request);

int OSMP_RemoveRequest(OSMP_REQUEST* request);

/**
 * Get the current shared memory pointer, for testing purpose only
 * @return the current shared memory pointer when the OSMP library is initialized else {@link NULL}
 */
SHARED_MEMORY* OSMP_Get_Shared_Memory();

#endif //OSMPRUN_OSMP_H
