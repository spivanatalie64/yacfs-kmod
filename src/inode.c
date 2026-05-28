/*
 * YAcFS kernel module — Inode operations
 */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/namei.h>
#include <linux/vfs.h>
#include <linux/cred.h>

extern const struct inode_operations yacfs_dir_inode_ops;
extern const struct file_operations yacfs_dir_ops;
extern const struct inode_operations yacfs_inode_ops;
extern const struct file_operations yacfs_file_ops;

struct dentry *yacfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int flags) {
    d_add(dentry, NULL);
    return NULL;
}
