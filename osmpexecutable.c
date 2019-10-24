//
// Created by Fabian, Moritz on 09.04.18.
//

#include "OSMP.h"

int main(int argc, char* argv[]) {
    if (OSMP_Init(&argc, &argv) == OSMP_ERROR) return EXIT_FAILURE;

    unsigned int rank;

    OSMP_Rank(&rank);

    unsigned int size;

    OSMP_Size(&size);

    char data[125];

    unsigned int source;

    size_t length;

    OSMP_Send(data, 125, (rank + 1) % size);

    printf("message send from %d to %d\n", rank, (rank + 1) % size);

    OSMP_Recv(data, 125, &source, &length);

    printf("message received for %d\n", rank);

    if (OSMP_Finalize() == OSMP_ERROR) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}