/*
 * YAcFS kernel module — File operations
 */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

static int yacfs_open(struct inode *inode, struct file *file) { return 0; }
static int yacfs_release(struct inode *inode, struct file *file) { return 0; }

static ssize_t yacfs_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) {
    return 0;  /* Read-only stub — pool integration coming in v3.1 */
}

static ssize_t yacfs_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) {
    return -EROFS;
}

const struct file_operations yacfs_file_ops = {
    .open   = yacfs_open,
    .release = yacfs_release,
    .read   = yacfs_read,
    .write  = yacfs_write,
    .llseek = generic_file_llseek,
};
