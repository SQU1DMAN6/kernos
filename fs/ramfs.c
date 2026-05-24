#include "vfs.h"
#include "memory.h"
#include "string.h"
#include "terminal.h"

static fs_node_t *ramfs_root = 0;

static void copy_name(
    char *dest,
    const char *src
)
{
    unsigned int i = 0;

    while (src[i]) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = 0;
}

static fs_node_t *copy_node(fs_node_t *node, fs_node_t *new_parent)
{
    fs_node_t *copy = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    if (!copy) return 0;

    *copy = *node;

    copy->parent = new_parent;
    copy->next = new_parent->children;
    new_parent->children = copy;

    if (node->type == FS_FILE && node->data) {
        unsigned int len = node->size;
        copy->data = kmalloc(len + 1);
        if (copy->data) {
            strcpy(copy->data, node->data);
        }
    }

    fs_node_t *child = node->children;
    copy->children = 0;

    while (child) {
        copy_node(child, copy);
        child = child->next;
    }

    return copy;
}

static int name_equals(
    const char *a,
    const char *b
)
{
    return strcmp(a, b);
}

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

static fs_node_t *find_child(
    fs_node_t *parent,
    const char *name
)
{
    fs_node_t *current = parent->children;

    while (current) {
        if (name_equals(current->name, name)) {
            return current;
        }

        current = current->next;
    }

    return 0;
}

static int unlink_child(
    fs_node_t *parent,
    fs_node_t *target
)
{
    fs_node_t *current = parent->children;
    fs_node_t *prev = 0;

    while (current) {
        if (current == target) {
            if (prev) {
                prev->next = current->next;
            } else {
                parent->children = current->next;
            }
            return 1;
        }

        prev = current;
        current = current->next;
    }

    return 0;
}

static fs_node_t *resolve_path(
    const char *path
)
{
    if (!path_is_ramfs(path)) {
        return 0;
    }

    const char *subpath = strip_mount(path);

    if (!subpath[0]) {
        return ramfs_root;
    }

    fs_node_t *current = ramfs_root;

    char segment[FS_MAX_NAME];
    unsigned int seg_i = 0;

    unsigned int i = 0;

    while (1) {
        char c = subpath[i];

        if (c == '/' || c == 0) {
            segment[seg_i] = 0;

            current =
                find_child(current, segment);

            if (!current) {
                return 0;
            }

            seg_i = 0;

            if (c == 0) {
                return current;
            }
        }
        else {
            segment[seg_i++] = c;
        }

        i++;
    }
}

static int split_parent_path(
    const char *path,
    char *parent_out,
    char *name_out
)
{
    unsigned int len = 0;

    while (path[len]) {
        len++;
    }

    if (len <= 5) {
        return 0;
    }

    int last_slash = -1;

    for (unsigned int i = 5; path[i]; i++) {
        if (path[i] == '/') {
            last_slash = i;
        }
    }

    if (last_slash == -1) {
        copy_name(
            parent_out,
            "RAM:/"
        );

        copy_name(
            name_out,
            strip_mount(path)
        );

        return 1;
    }

    unsigned int i;

    for (i = 0; i < (unsigned int)last_slash; i++) {
        parent_out[i] = path[i];
    }

    parent_out[i] = 0;

    copy_name(
        name_out,
        path + last_slash + 1
    );

    return 1;
}

void fs_init(void)
{
    ramfs_root =
        (fs_node_t*)kmalloc(sizeof(fs_node_t));

    ramfs_root->name[0] = 0;

    ramfs_root->type = FS_DIRECTORY;

    ramfs_root->data = 0;
    ramfs_root->size = 0;

    ramfs_root->permissions = 0xFFFF;

    ramfs_root->parent = 0;
    ramfs_root->children = 0;
    ramfs_root->next = 0;
}

int fs_create(const char *path)
{
    char parent_path[FS_MAX_PATH];
    char name[FS_MAX_NAME];

    if (!split_parent_path(
        path,
        parent_path,
        name
    )) {
        return 0;
    }

    fs_node_t *parent =
        resolve_path(parent_path);

    if (!parent ||
        parent->type != FS_DIRECTORY)
    {
        return 0;
    }

    if (find_child(parent, name)) {
        return 0;
    }

    fs_node_t *node =
        (fs_node_t*)kmalloc(sizeof(fs_node_t));

    if (!node) {
        return 0;
    }

    copy_name(node->name, name);

    node->type = FS_FILE;

    node->data = 0;
    node->size = 0;

    node->permissions = 0xFFFF;

    node->parent = parent;
    node->children = 0;

    node->next = parent->children;
    parent->children = node;

    return 1;
}

int fs_mkdir(const char *path)
{
    char parent_path[FS_MAX_PATH];
    char name [FS_MAX_NAME];

    if (!split_parent_path(
        path,
        parent_path,
        name
    )) {
        return 0;
    }

    fs_node_t *parent =
        resolve_path(parent_path);

    if (!parent) {
        return 0;
    }

    if (find_child(parent, name)) {
        return 0;
    }

    fs_node_t *node =
        (fs_node_t*)kmalloc(sizeof(fs_node_t));

    if (!node) {
        return 0;
    }

    copy_name(node->name, name);

    node->type = FS_DIRECTORY;

    node->data = 0;
    node->size = 0;

    node->permissions = 0xFFFF;

    node->parent = parent;
    node->children = 0;

    node->next = parent->children;
    parent->children = node;

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
        resolve_path(path);

    if (!node) {
        return 0;
    }

    unsigned int len = 0;

    while (data[len]) {
        len++;
    }

    if (node->data) {
        kfree(node->data);
    }

    node->data = (char*)kmalloc(len + 1);

    if (!node->data) {
        return 0;
    }

    for (unsigned int i = 0; i < len; i++) {
        node->data[i] = data[i];
    }

    node->data[len] = 0;

    node->size = len;

    return 1;
}

char *fs_read(const char *path)
{
    if (!path_is_ramfs(path)) {
        return 0;
    }

    fs_node_t *node =
        resolve_path(path);

    if (!node) {
        return 0;
    }

    return node->data;
}

int fs_remove(const char *path)
{
    fs_node_t *node = resolve_path(path);
    if (!node || !node->parent) {
        return 0;
    }

    if (!unlink_child(node->parent, node)) {
        return 0;
    }

    if (node->data) {
        kfree(node->data);
    }

    kfree(node);
    return 1;
}

int fs_rename(const char *path, const char *new_name)
{
    fs_node_t *node = resolve_path(path);
    if (!node) return 0;

    copy_name(node->name, new_name);
    return 1;
}

int fs_move(const char *src, const char *dst)
{
    fs_node_t *node = resolve_path(src);
    if (!node || !node->parent) return 0;

    char parent_path[FS_MAX_PATH];
    char name[FS_MAX_NAME];

    if (!split_parent_path(dst, parent_path, name)) {
        return 0;
    }

    fs_node_t *new_parent = resolve_path(parent_path);
    if (!new_parent) return 0;

    unlink_child(node->parent, node);

    node->parent = new_parent;
    copy_name(node->name, name);

    node->next = new_parent->children;
    new_parent->children = node;

    return 1;
}

int fs_copy(
    const char *src,
    const char *dst
)
{
    fs_node_t *node =
        resolve_path(src);

    if (!node) {
        return 0;
    }

    char parent_path[FS_MAX_PATH];
    char name[FS_MAX_NAME];

    if (!split_parent_path(
        dst,
        parent_path,
        name
    )) {
        return 0;
    }

    fs_node_t *parent =
        resolve_path(parent_path);

    if (!parent) {
        return 0;
    }

    fs_node_t *copy =
        copy_node(node, parent);

    if (!copy) {
        return 0;
    }

    copy_name(copy->name, name);

    return 1;
}

void fs_list_path(const char *path)
{
    fs_node_t *dir =
        resolve_path(path);

    if (!dir) {
        kprint("Directory not found\n");
        return;
    }

    fs_node_t *current =
        dir->children;

    while (current) {
        if (current->type == FS_DIRECTORY) {
            kprint("[D] ");
        } else {
            kprint("[*] ");
        }

        kprint(current->name);
        kprintln();

        current = current->next;
    }
}

void fs_list(void)
{
    fs_list_path("RAM:/");
}
