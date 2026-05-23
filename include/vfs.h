#ifndef FS_H
#define FS_H

#define FS_MAX_NAME 64

typedef struct fs_node {
    char name[FS_MAX_NAME];

    char *data;
    unsigned int size;

    struct fs_node *next;
} fs_node_t;

void fs_init(void);

int fs_create(const char *path);

int fs_write(
    const char *path,
    const char *data
);

char *fs_read(const char *path);

void fs_list(void);

#endif