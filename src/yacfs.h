#ifndef _YACFS_H
#define _YACFS_H

#include <linux/types.h>
#include <linux/fs.h>

#define YACFS_SUPER_MAGIC   0x59414346UL
#define YACFS_BLOCK_MAGIC   0x5A4653
#define YACFS_ROOT_INO      1
#define YACFS_POOL_MAX      4096
#define YACFS_INODE_SIZE    72
#define YACFS_BLOCK_HDR_SIZE 20
#define YACFS_DIRENT_BASE   11

struct yacfs_sb_info {
    char pool_path[YACFS_POOL_MAX];
    int  root_loaded;
};

struct yacfs_inode_disk {
    __u64 ino;
    __u64 size;
    __u64 mtime;
    __u64 ctime;
    __u32 mode;
    __u32 uid;
    __u32 gid;
    __u32 nblocks;
    __u32 block_size;
    __u32 checksum_type;
    __u32 compress_type;
    __u32 nlink;
    __u32 pad;
    __u64 xattr_block;
} __attribute__((packed));

struct yacfs_block_hdr {
    __u32 magic;
    __u32 checksum;
    __u32 orig_size;
    __u32 comp_size;
    __u8  compress;
    __u8  checksum_type;
    __u16 pad;
} __attribute__((packed));

struct yacfs_dirent {
    __u16 name_len;
    __u64 ino;
    __u8  type;
    char  name[];
} __attribute__((packed));

int yacfs_load_root(struct super_block *sb);
int yacfs_read_ino(struct super_block *sb, u64 ino, struct yacfs_inode_disk *disk);
int yacfs_read_ino_blocks(struct super_block *sb, u64 ino, struct yacfs_inode_disk *disk, __u64 **blocks);
int yacfs_read_block(struct super_block *sb, u64 hash, void *buf, size_t *size);
int yacfs_fill_inode(struct inode *inode, struct yacfs_inode_disk *disk);

struct dentry *yacfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int flags);

extern const struct super_operations yacfs_super_ops;
extern const struct inode_operations yacfs_dir_inode_ops;
extern const struct inode_operations yacfs_file_inode_ops;
extern const struct file_operations yacfs_dir_ops;
extern const struct file_operations yacfs_file_ops;

#endif
