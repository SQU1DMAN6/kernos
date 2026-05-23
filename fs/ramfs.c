#include "vfs.h"
#include "memory.h"
#include "string.h"
#include "terminal.h"

static fs_node_t *ramfs_root = 0;

static int path_is_ramfs(const char *path)
{
    return
        path[0] == 'R' &&
        path[1] == 'A' &&
        path[2] == 'M' &&
        path[3] == ':' &&
        path[4] == '/';
}

static char *strip_mount(const char *path)
{
    return path + 5;
}

static fs_node_t *find_node(const char *name)
{
    fs_node_t *current = ramfs_root;

    while (current) {
        if (strcmp(current->name, name)) {
            return current;
        }

        current = current->next;
    }

    return 0;
}

void fs_init(void)
{
    ramfs_root = 0;
}

int fs_create(const char *path)
{
    if (!path_is_ramfs(path)) {
        return 0;
    }

    const char *name = strip_mount(path);

    if (find_node(name)) {
        return 0;
    }

    fs_node_t *node =
        (fs_node_t*)kmalloc(sizeof(fs_node_t));

    if (!node) {
        return 0;
    }

    strcpy(node->name, name);

    node->data = 0;
    node->size = 0;

    node->next = ramfs_root;
    ramfs_root = node;

    return 1;
}

int fs_write(
    const char *path,
    const char *data
)
{
    if (!path_is_ramfs(path)) {
        return 0;
    }

    fs_node_t *node =
        find_node(strip_mount(path));

    if (!node) {
        return 0;
    }

    unsigned int len = 0;

    while (data[len]) {
        len++;
    }

    node->data = (char*)kmalloc(len + 1);

    if (!node->data) {
        return 0;
    }

    strcpy(node->data, data);

    node->size = len;

    return 1;
}

char *fs_read(const char *path)
{
    if (!path_is_ramfs(path)) {
        return 0;
    }

    fs_node_t *node =
        find_node(strip_mount(path));

    if (!node) {
        return 0;
    }

    return node->data;
}

void fs_list(void)
{
    fs_node_t *current = ramfs_root;

    while(current) {
        kprint("RAM:/");
        kprint(current->name);
        kprintln();

        current = current->next;
    }
}