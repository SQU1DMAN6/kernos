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

int strlen(const char *str)
{
    int len = 0;

    while (str[len]) {
        len++;
    }

    return len;
}

int strncmp(
    const char *a,
    const char *b,
    int n
)
{
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }

        if (a[i] == 0 || b[i] == 0) {
            break;
        }
    }

    return 1;
}
