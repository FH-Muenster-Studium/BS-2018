//
// Created by Fabian Terhorst on 09.05.18.
//

#ifndef OSMPRUN_INT_TO_STRING_H
#define OSMPRUN_INT_TO_STRING_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) {
        *result = '\0';
        return result;
    }

    char* ptr = result, * ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 +
                                                                                           (tmp_value - value * base)];
    } while (value);

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

char* intToString(int i, char** ptr) {
    if (i == 0) {
        char* string = malloc((sizeof(char) * 1) + 1);
        strcpy(string, "0\0");
        *ptr = string;
        return string;
    }
    size_t size = (size_t) (ceil(log10(i)) + 1);
    *ptr = malloc((size * sizeof(char)) + 1);
    return itoa(i, *ptr, 10);
}

#endif //OSMPRUN_INT_TO_STRING_H
