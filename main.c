#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void error_routine(char* error_message) {
    printf("%s", error_message);
    exit(1);
}

int main() {
    pid_t pid1;

    if ((pid1 = fork()) < 0) {
        error_routine("fork error");
    } else if (pid1 == 0) { // Child process
        execlp("/Users/fabianterhorst/CLionProjects/OSMP/cmake-build-debug/echoall", /* File path*/
                           "Test1", /* Argument 1*/
                           "Test2", /* Argument 2*/
                           NULL); /* NULL Argument*/
        error_routine("execle error");
    } else { // Main process
        if (waitpid(pid1, NULL, 0) != pid1) {
            error_routine("waitpid error");
        }
    }
    return 0;
}