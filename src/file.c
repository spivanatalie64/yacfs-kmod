#include "yacfs.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static int yacfs_open(struct inode *inode, struct file *file) { return 0; }
static int yacfs_release(struct inode *inode, struct file *file) { return 0; }

static ssize_t yacfs_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(file);
    struct super_block *sb = inode->i_sb;
    struct yacfs_inode_disk disk;
    __u64 *blocks = NULL;
    char *data = NULL;
    ssize_t total = 0;
    loff_t pos = *ppos;
    int ret, i;
    u32 block_size;
    u32 block_idx;
    u32 block_off;

    if (pos >= inode->i_size)
        return 0;
    if (pos + len > inode->i_size)
        len = inode->i_size - pos;

    ret = yacfs_read_ino_blocks(sb, inode->i_ino, &disk, &blocks);
    if (ret)
        return ret;

    block_size = disk.block_size;
    block_idx = pos / block_size;
    block_off = pos % block_size;

    for (i = block_idx; i < disk.nblocks && total < len; i++) {
        size_t data_sz = block_size;
        size_t copy_sz;

        data = kmalloc(block_size, GFP_KERNEL);
        if (!data) {
            ret = -ENOMEM;
            break;
        }

        ret = yacfs_read_block(sb, blocks[i], data, &data_sz);
        if (ret) {
            kfree(data);
            break;
        }

        copy_sz = min_t(size_t, len - total, data_sz - block_off);
        if (copy_to_user(buf + total, data + block_off, copy_sz)) {
            kfree(data);
            ret = -EFAULT;
            break;
        }
        total += copy_sz;
        block_off = 0;
        kfree(data);
        data = NULL;
    }

    kfree(data);
    kfree(blocks);

    if (ret && total == 0)
        return ret;

    *ppos += total;
    return total;
}

const struct inode_operations yacfs_file_inode_ops = {
};

const struct file_operations yacfs_file_ops = {
    .open    = yacfs_open,
    .release = yacfs_release,
    .read    = yacfs_read,
    .llseek  = generic_file_llseek,
};
