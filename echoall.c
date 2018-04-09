/**
 * 09.03.2018 echoall.c fabian
 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int i = 0;
    char* current;
    while ((current = argv[i]) != NULL) {
        printf("%s\n", current);
        i++;
    }
    for (i = 0;i < 10;i++) {
        printf("Child\n");
        sleep(1);
    }
    return 0;
}