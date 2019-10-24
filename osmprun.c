//
// Created by Fabian, Moritz on 09.04.18.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <math.h>
#include "OSMP.h"
#include <semaphore.h>
#include <errno.h>
#include "int_to_string.h"

//#define FABIAN

#ifdef __linux__

#include <sys/wait.h>

#ifdef FABIAN
#define PATH "/home/parallels/Desktop/ParallelsSharedFolders/Home/CLionProjects/bsp09/cmake-build-debug/"
#elif MORITZ
#define PATH "/home/ms/m/ms242254/Dokumente/BS/cmake-build-debug/"
#else
#define PATH "/home/ms/m/ms242254/Dokumente/BS/cmake-build-debug/"
#endif
#endif

#ifdef __APPLE__
#define PATH "/Users/fabianterhorst/CLionProjects/bsp09/cmake-build-debug/"
#endif

int open_shared_memory();

int create_shared_memory();

int read_shared_memory();

void createChildProcesses(unsigned int count, char* executable, int fd, sem_t* sem);

void error_routine(char* error_message);

void error_routine_status_code(char* error_message, int status);

int main(int argc, char* argv[]) {
    if (argc != 3) return EXIT_FAILURE;
    char* end;
    long processCount = strtol(argv[1], &end, 10);
    printf("count %ld\n", processCount);

    char* child_executable = argv[2];
    printf("exe %s\n", child_executable);

    sem_unlink(MEMORY_READ_SEMAPHORE_NAME);

    sem_unlink(MEMORY_WRITE_SEMAPHORE_NAME);

    sem_unlink(MESSAGES_SEMAPHORE_NAME);

    sem_unlink(MEMORY_INIT_SEMAPHORE_NAME);

    for (unsigned int i = 0; i < processCount; i++) {
        char* ptr;
        char* sem_rank = intToString(i, &ptr);
        char* sem_name = malloc(sizeof(char) * (strlen(MESSAGES_SEMAPHORE_NAME_PREFIX) + strlen(sem_rank)) + 1);
        strcpy(sem_name, MESSAGES_SEMAPHORE_NAME_PREFIX);
        strcat(sem_name, sem_rank);
        sem_unlink(sem_name);
        free(sem_name);
        sem_name = malloc(sizeof(char) * (strlen(MESSAGES_SEMAPHORE_LIMIT_NAME_PREFIX) + strlen(sem_rank)) + 1);
        strcpy(sem_name, MESSAGES_SEMAPHORE_LIMIT_NAME_PREFIX);
        strcat(sem_name, sem_rank);
        sem_unlink(sem_name);
        free(ptr);
        free(sem_name);
    }

    shm_unlink(SHARED_MEMORY_NAME);

    int fd;

    if ((fd = read_shared_memory()) == EXIT_FAILURE) {
        exit(EXIT_FAILURE);
    }

    sem_t* sem;

    if ((sem = sem_open(MEMORY_INIT_SEMAPHORE_NAME, O_CREAT, 0644, 0)) == NULL) {
        error_routine("sem_init() error");
    }

    createChildProcesses((unsigned int) processCount, child_executable, fd, sem);

    //shm_unlink(SHARED_MEMORY_NAME); //TODO: unlink shared memory when all processes are killed
    return 0;
}

void createChildProcesses(unsigned int count, char* executable, int fd, sem_t* sem) {
    pid_t pid;
    size_t shared_memory_size = sizeof(SHARED_MEMORY) + (sizeof(PROCESS_HEADER) * count);
    SHARED_MEMORY* shared_memory = malloc(shared_memory_size);
    if (!shared_memory) {
        error_routine("shared_memory malloc() failed");
        return;
    }
    shared_memory->header.size = count;
    shared_memory->header.first_last_free.first = 0;
    shared_memory->header.first_last_free.last = OSMP_MAX_SLOTS - 1;

    // Set next index for all except last
    for (int i = 0, last = OSMP_MAX_SLOTS - 1; i < last; i++) {
        shared_memory->messages[i].next = i + 1;
    }
    // Last message doesn't has an next free message index for now
    shared_memory->messages[OSMP_MAX_SLOTS - 1].next = -1;

    for (int i = 0; i < count; i++) {
        if ((pid = fork()) < 0) {
            error_routine("fork error");
            break;
        } else if (pid == 0) { // Child
            printf("Child:%d\n", i);
            char* executableFullPath = malloc(strlen(PATH) + strlen(executable) + 1);// +1 for null terminating byte
            strcpy(executableFullPath, PATH);
            strcat(executableFullPath, executable);
            printf("sem_wait() started\n");
            sem_wait(sem);
            printf("sem_wait() finished\n");
            sem_post(sem);

            if (i == count - 1) {
                sem_close(sem);
                sem_unlink(MEMORY_INIT_SEMAPHORE_NAME);
            }

            int error = execlp(executableFullPath, /* File path*/
                               "",
                               NULL); /* NULL Argument*/
            free(executableFullPath);
            error_routine_status_code("execle error", error);
            break;
        } else { // Parent
            printf("Parent created child:%d\n", pid);
            shared_memory->header.processes[i].process_id = pid;
            shared_memory->header.processes[i].first_last_msg.first = -1;
            shared_memory->header.processes[i].first_last_msg.last = -1;
            /*if (waitpid(pid, NULL, 0) != pid) {
                error_routine("waitpid error");
            }*/
        }
    }

    // Specify shared memory size
    if (ftruncate(fd, (off_t) shared_memory_size) < 0) {
        error_routine("ftruncate failed");
        return;
    }

    // Map shared memory content to write it
    void* mmap_address = mmap(NULL, shared_memory_size,
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_address == MAP_FAILED) {
        error_routine("shared_memory mmap() failed");
        return;
    }

    // Copy shared memory struct to it
    memcpy(mmap_address, shared_memory, shared_memory_size);

    // Unmap shared memory content
    if (munmap(mmap_address, shared_memory_size) == -1) {
        error_routine("shared_memory unmap() failed");
        return;
    }

    free(shared_memory);

    printf("sem_post() finished\n");
    sem_post(sem);
}

void error_routine(char* error_message) {
    printf("%s\n", error_message);
    exit(EXIT_FAILURE);
}

void error_routine_status_code(char* error_message, int status) {
    printf("%s %d\n", error_message, status);
    exit(status);
}

int create_shared_memory() {
    return shm_open(SHARED_MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, /*0640*/S_IWUSR | S_IRUSR);
}

int open_shared_memory() {
    return shm_open(SHARED_MEMORY_NAME, O_RDWR, 0);
}

int read_shared_memory() {
    shm_unlink(SHARED_MEMORY_NAME);
    int fd = create_shared_memory();
    if (fd >= 0) {
        printf("fd created %d\n", fd);
    } else {
        fd = open_shared_memory();
        if (fd >= 0) {
            printf("fd already exists %d\n", fd);
        } else {
            printf("%s", "fd error\n");
            return EXIT_FAILURE;
        }
    }
    return fd;
}

/*void createChild(long count) {
    if (count == 0) return;
    pid_t pid1;
    if((pid1 = fork()) > 0){
        printf("Main\n");
        createChild(--count);
    }
}*/
