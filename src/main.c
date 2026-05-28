/*
 * YAcFS kernel module — Main entry point
 *
 * Register "yacfs" filesystem type with the VFS using the new mount API.
 * Mount: mount -t yacfs -o pool=/path/to/pool /mnt/yacfs
 *
 * Kernel-level YAcFS implementation bypassing FUSE entirely.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>
#include <linux/pagemap.h>
#include <linux/mount.h>
#include <linux/parser.h>
#include <linux/statfs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/buffer_head.h>
#include <linux/cred.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Natalie Spiva, Darren Clift — AcreetionOS");
MODULE_DESCRIPTION("YAcFS v3 — Yet Another common File System (kernel module)");
MODULE_VERSION("3.0.0");

#define YACFS_POOL_MAX 4096

/* Per-superblock YAcFS state */
struct yacfs_sb_info {
    char pool_path[YACFS_POOL_MAX];
    uint64_t next_ino;
};

/* Mount options */
struct yacfs_fs_context {
    char *pool_path;
};

extern const struct super_operations yacfs_super_ops;

enum {
    OPT_POOL,
};

static const struct fs_parameter_spec yacfs_params[] = {
    fsparam_string("pool", OPT_POOL),
    {}
};

static int yacfs_parse_param(struct fs_context *fc, struct fs_parameter *param) {
    struct yacfs_fs_context *ctx = fc->fs_private;
    struct fs_parse_result result;
    int opt;

    opt = fs_parse(fc, yacfs_params, param, &result);
    if (opt < 0) return opt;

    switch (opt) {
    case OPT_POOL:
        kfree(ctx->pool_path);
        ctx->pool_path = kstrdup(param->string, GFP_KERNEL);
        if (!ctx->pool_path) return -ENOMEM;
        break;
    }

    return 0;
}

static int yacfs_get_tree(struct fs_context *fc) {
    struct yacfs_fs_context *ctx = fc->fs_private;
    if (!ctx->pool_path) {
        errorf(fc, "YAcFS requires pool= option");
        return -EINVAL;
    }

    struct super_block *sb = sget_fc(fc, NULL, NULL);
    if (IS_ERR(sb)) return PTR_ERR(sb);

    struct yacfs_sb_info *sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
    if (!sbi) { deactivate_locked_super(sb); return -ENOMEM; }

    strncpy(sbi->pool_path, ctx->pool_path, YACFS_POOL_MAX - 1);
    sbi->next_ino = 2;
    sb->s_fs_info = sbi;
    sb->s_op = &yacfs_super_ops;
    sb->s_magic = 0x59414346;  /* "YACF" */
    sb->s_blocksize = 65536;
    sb->s_blocksize_bits = 16;

    /* Create root inode */
    struct inode *root = new_inode(sb);
    if (!root) {
        kfree(sbi);
        deactivate_locked_super(sb);
        return -ENOMEM;
    }
    root->i_ino = 1;
    root->i_mode = S_IFDIR | 0755;
    root->i_uid = current_fsuid();
    root->i_gid = current_fsgid();
    root->i_size = 0;
    root->i_atime = root->i_mtime = root->i_ctime = current_time(root);
    root->i_op = &simple_dir_inode_operations;
    root->i_fop = &simple_dir_operations;

    sb->s_root = d_make_root(root);
    if (!sb->s_root) {
        kfree(sbi);
        deactivate_locked_super(sb);
        return -ENOMEM;
    }

    fc->root = dget(sb->s_root);
    return 0;
}

static void yacfs_free_fc(struct fs_context *fc) {
    struct yacfs_fs_context *ctx = fc->fs_private;
    kfree(ctx->pool_path);
    kfree(ctx);
}

static const struct fs_context_operations yacfs_context_ops = {
    .parse_param = yacfs_parse_param,
    .get_tree    = yacfs_get_tree,
    .free        = yacfs_free_fc,
};

static int yacfs_init_fs_context(struct fs_context *fc) {
    struct yacfs_fs_context *ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx) return -ENOMEM;
    fc->fs_private = ctx;
    fc->ops = &yacfs_context_ops;
    return 0;
}

static void yacfs_kill_sb(struct super_block *sb) {
    struct yacfs_sb_info *sbi = sb->s_fs_info;
    kfree(sbi);
    sb->s_fs_info = NULL;
    kill_anon_super(sb);
}

static struct file_system_type yacfs_fs_type = {
    .name              = "yacfs",
    .fs_flags          = FS_USERNS_MOUNT,
    .init_fs_context   = yacfs_init_fs_context,
    .parameters        = yacfs_params,
    .kill_sb           = yacfs_kill_sb,
    .owner             = THIS_MODULE,
};

static int __init yacfs_module_init(void) {
    int ret = register_filesystem(&yacfs_fs_type);
    if (ret == 0)
        pr_info("YAcFS v3.0 loaded — kernel-mode filesystem\n");
    return ret;
}

static void __exit yacfs_module_exit(void) {
    unregister_filesystem(&yacfs_fs_type);
    pr_info("YAcFS v3.0 unloaded\n");
}

module_init(yacfs_module_init);
module_exit(yacfs_module_exit);
