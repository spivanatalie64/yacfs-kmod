#include "yacfs.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/statfs.h>

static void yacfs_put_super(struct super_block *sb)
{
    struct yacfs_sb_info *sbi = sb->s_fs_info;
    kfree(sbi);
    sb->s_fs_info = NULL;
}

static int yacfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    buf->f_type = YACFS_SUPER_MAGIC;
    buf->f_bsize = 65536;
    buf->f_blocks = 0;
    buf->f_bfree = 0;
    buf->f_bavail = 0;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_namelen = 255;
    return 0;
}

static int yacfs_sync_fs(struct super_block *sb, int wait)
{
    return 0;
}

static struct inode *yacfs_alloc_inode(struct super_block *sb)
{
    return new_inode(sb);
}

static void yacfs_evict_inode(struct inode *inode)
{
    clear_inode(inode);
}

const struct super_operations yacfs_super_ops = {
    .put_super     = yacfs_put_super,
    .alloc_inode   = yacfs_alloc_inode,
    .evict_inode   = yacfs_evict_inode,
    .statfs        = yacfs_statfs,
    .sync_fs       = yacfs_sync_fs,
};
