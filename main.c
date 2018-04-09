/**
 * 09.03.2018 main.c fabian
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void error_routine(char* error_message) {
    printf("%s", error_message);
    exit(1);
}

void error_routine_status_code(char* error_message, int status) {
    printf("%s %d", error_message, status);
    exit(1);
}

int main() {
    pid_t pid1;

    if ((pid1 = fork()) < 0) {
        error_routine("fork error");
    } else if (pid1 == 0) { // Child process
        int error = execlp("/Users/fabianterhorst/CLionProjects/OSMP/cmake-build-debug/echoall", /* File path*/
                           "Test1", /* Argument 1*/
                           "Test2", /* Argument 2*/
                           NULL); /* NULL Argument*/
        error_routine_status_code("execle error", error);
    } else { // Main process
        for (int i = 0;i < 5;i++) {
            printf("Parent\n");
            sleep(1);
        }
        if (waitpid(pid1, NULL, 0) != pid1) {
            error_routine("waitpid error");
        }
        printf("Child finished");
    }
    return 0;
}