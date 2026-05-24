#ifndef FS_H
#define FS_H

#define FS_MAX_NAME 128
#define FS_MAX_PATH 512

typedef enum {
    FS_FILE,
    FS_DIRECTORY
} fs_type_t;

typedef struct fs_node {
    char name[FS_MAX_NAME];

    fs_type_t type;

    char *data;
    unsigned int size;

    unsigned int permissions;

    struct fs_node *parent;
    struct fs_node *children;

    struct fs_node *next;
} fs_node_t;

void fs_init(void);

int fs_create(const char *path);
int fs_mkdir (const char *path);
int fs_write(
    const char *path,
    const char *data
);
char *fs_read(const char *path);
int fs_remove(const char *path);
int fs_move(const char *src, const char *dst);
int fs_copy(const char *src, const char *dst);

void fs_list(void);
void fs_list_path(const char *path);

#endif