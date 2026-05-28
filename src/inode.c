#include "yacfs.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/namei.h>
#include <linux/cred.h>

struct dentry *yacfs_lookup(struct inode *parent_dir, struct dentry *dentry, unsigned int flags)
{
    struct super_block *sb = parent_dir->i_sb;
    struct yacfs_sb_info *sbi = sb->s_fs_info;
    struct yacfs_inode_disk disk;
    __u64 *blocks = NULL;
    struct inode *inode = NULL;
    char *data = NULL;
    int ret, i;
    off_t off;

    /* Defer pool I/O — load root on first access, not during mount */
    if (!READ_ONCE(sbi->root_loaded)) {
        ret = yacfs_load_root(sb);
        if (ret)
            goto out;
    }

    ret = yacfs_read_ino_blocks(sb, parent_dir->i_ino, &disk, &blocks);
    if (ret)
        goto out;
    if (!S_ISDIR(disk.mode)) {
        ret = -ENOTDIR;
        goto out;
    }

    for (i = 0; i < disk.nblocks; i++) {
        size_t data_sz = disk.block_size;

        kfree(data);
        data = kmalloc(disk.block_size, GFP_KERNEL);
        if (!data) {
            ret = -ENOMEM;
            goto out;
        }

        ret = yacfs_read_block(sb, blocks[i], data, &data_sz);
        if (ret)
            continue;

        off = 0;
        while (off + sizeof(__u16) <= (off_t)data_sz) {
            struct yacfs_dirent *e = (struct yacfs_dirent *)(data + off);
            if (off + sizeof(__u16) + e->name_len + sizeof(__u64) + sizeof(__u8) > (off_t)data_sz)
                break;

            if (e->name_len == dentry->d_name.len &&
                !memcmp(e->name, dentry->d_name.name, e->name_len)) {
                inode = iget_locked(sb, e->ino);
                if (!inode) {
                    ret = -ENOMEM;
                    goto out;
                }
                if (inode_state_read_once(inode) & I_NEW) {
                    ret = yacfs_read_ino(sb, e->ino, &disk);
                    if (!ret) {
                        yacfs_fill_inode(inode, &disk);
                    }
                    unlock_new_inode(inode);
                    if (ret) {
                        iput(inode);
                        inode = NULL;
                        goto out;
                    }
                }
                d_add(dentry, inode);
                goto out;
            }
            off += sizeof(__u16) + e->name_len + sizeof(__u64) + sizeof(__u8);
        }
    }

out:
    kfree(data);
    kfree(blocks);
    if (!inode)
        d_add(dentry, NULL);
    return NULL;
}
