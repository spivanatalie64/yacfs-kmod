#include "yacfs.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cred.h>

struct dentry *yacfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int flags);

static int yacfs_readdir(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);
    struct super_block *sb = inode->i_sb;
    struct yacfs_sb_info *sbi = sb->s_fs_info;

    /* Defer pool I/O — load root on first access, not during mount */
    if (!READ_ONCE(sbi->root_loaded)) {
        int ret = yacfs_load_root(sb);
        if (ret)
            return ret;
        /* If we're reading root and it just got loaded, re-get inode */
        if (inode->i_ino == YACFS_ROOT_INO)
            inode = sb->s_root->d_inode;
    }
    struct yacfs_inode_disk disk;
    __u64 *blocks = NULL;
    char *data = NULL;
    int ret = 0, i;
    u64 idx = 0;
    off_t off;

    if (!dir_emit_dots(file, ctx))
        return 0;

    ret = yacfs_read_ino_blocks(sb, inode->i_ino, &disk, &blocks);
    if (ret)
        return ret;

    for (i = 0; i < disk.nblocks; i++) {
        size_t data_sz = disk.block_size;

        kfree(data);
        data = kmalloc(disk.block_size, GFP_KERNEL);
        if (!data) {
            ret = -ENOMEM;
            break;
        }

        ret = yacfs_read_block(sb, blocks[i], data, &data_sz);
        if (ret)
            break;

        off = 0;
        while (off + YACFS_DIRENT_BASE <= data_sz) {
            struct yacfs_dirent *de = (struct yacfs_dirent *)(data + off);
            unsigned char d_type;

            if (de->name_len == 0 ||
                off + YACFS_DIRENT_BASE + de->name_len > data_sz)
                break;

            if (idx + 2 >= ctx->pos) {
                switch (de->type) {
                case 1:  d_type = DT_REG;  break;
                case 2:  d_type = DT_DIR;  break;
                case 3:  d_type = DT_CHR;  break;
                case 4:  d_type = DT_BLK;  break;
                case 5:  d_type = DT_FIFO; break;
                case 6:  d_type = DT_SOCK; break;
                case 7:  d_type = DT_LNK;  break;
                default: d_type = DT_UNKNOWN;
                }

                if (!dir_emit(ctx, de->name, de->name_len, de->ino, d_type)) {
                    goto out;
                }
                ctx->pos++;
            }
            idx++;
            off += YACFS_DIRENT_BASE + de->name_len;
        }
    }

out:
    kfree(data);
    kfree(blocks);
    return ret;
}

const struct inode_operations yacfs_dir_inode_ops = {
    .lookup = yacfs_lookup,
};

const struct file_operations yacfs_dir_ops = {
    .iterate_shared = yacfs_readdir,
};
