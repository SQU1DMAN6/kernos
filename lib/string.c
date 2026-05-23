#include "string.h"

int strcmp(const char *a, const char *b)
{
    unsigned int i = 0;

    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

void strcpy(char *dest, const char *src)
{
    unsigned int i = 0;

    while (src[i]) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = 0;
}