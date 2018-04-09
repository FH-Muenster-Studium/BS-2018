#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int i = 0;
    char* current;
    while ((current = argv[i]) != NULL) {
        printf("%s\n", current);
        i++;
    }
    return 0;
}