/*
 * YAcFS kernel module — Directory operations
 */
#include <linux/kernel.h>
#include <linux/fs.h>

extern struct dentry *yacfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int flags);

static int yacfs_readdir(struct file *file, struct dir_context *ctx) {
    return 0;
}

const struct inode_operations yacfs_dir_inode_ops = {
    .lookup = yacfs_lookup,
};

const struct file_operations yacfs_dir_ops = {
    .iterate_shared = yacfs_readdir,
};
