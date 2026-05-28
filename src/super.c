/*
 * YAcFS kernel module — Super block operations
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/statfs.h>

static int yacfs_statfs(struct dentry *dentry, struct kstatfs *buf) {
    buf->f_type = 0x59414346;
    buf->f_bsize = 65536;
    buf->f_blocks = 0;
    buf->f_bfree = 0;
    buf->f_bavail = 0;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_namelen = 255;
    return 0;
}

static int yacfs_sync_fs(struct super_block *sb, int wait) {
    return 0;
}

static struct inode *yacfs_alloc_inode(struct super_block *sb) {
    return new_inode(sb);
}

static void yacfs_evict_inode(struct inode *inode) {
    truncate_inode_pages(&inode->i_data, 0);
    clear_inode(inode);
}

const struct super_operations yacfs_super_ops = {
    .alloc_inode    = yacfs_alloc_inode,
    .evict_inode    = yacfs_evict_inode,
    .statfs         = yacfs_statfs,
    .sync_fs        = yacfs_sync_fs,
};
