# YAcFS v3 — Kernel Module

**Yet Another common File System** — a copy-on-write, content-addressed,
checksummed filesystem for Linux.

This kernel module provides **read-only mount support** for YAcFS v3 storage
pools. It registers the `yacfs` filesystem type and translates kernel VFS
operations into direct reads of the on-disk pool layout.

---

## Building

Prerequisites: Linux kernel headers, `make`, and a working C compiler
(GCC or Clang).

```sh
make
```

This produces `yacfs.ko`.  The module is also built automatically against the
running kernel via the symlink at `/lib/modules/$(uname -r)/build`.

### Load the module

```sh
sudo insmod yacfs.ko
# or, after copying to the modules directory:
sudo modprobe yacfs
```

### Unload

```sh
sudo rmmod yacfs
```

---

## Mounting

```sh
sudo mount -t yacfs -o pool=/path/to/pool /mnt/yacfs
```

The `pool=` option is **required** and must point to a directory containing a
valid YAcFS v3 pool (i.e. with `meta/` and `blocks/` subdirectories).

To verify:

```sh
mount | grep yacfs
ls -la /mnt/yacfs
```

---

## On-Disk Format

A YAcFS v3 pool is a plain directory tree on an existing filesystem:

```
<pool>/
├── meta/
│   └── <20-digit-ino>.ino      # inode files
└── blocks/
    └── <16-hex-hash>.blk       # block files
```

### Inodes (`meta/<ino>.ino`)

Each inode file begins with a fixed 72-byte header (`struct yacfs_inode_disk`):

| Offset | Size | Field          | Description                     |
|--------|------|----------------|---------------------------------|
| 0      | 8    | ino            | Inode number                    |
| 8      | 8    | size           | File size in bytes              |
| 16     | 8    | mtime          | Modification time (Unix sec)    |
| 24     | 8    | ctime          | Change time (Unix sec)          |
| 32     | 4    | mode           | File mode (S_I* flags)          |
| 36     | 4    | uid            | Owner UID                       |
| 40     | 4    | gid            | Owner GID                       |
| 44     | 4    | nblocks        | Number of data blocks           |
| 48     | 4    | block_size     | Logical block size (bytes)      |
| 52     | 4    | checksum_type  | Checksum algorithm for blocks   |
| 56     | 4    | compress_type  | Compression algorithm for blocks|
| 60     | 4    | nlink          | Hard link count                 |
| 64     | 8    | xattr_block    | Extended attribute block hash   |

Immediately after the header, an array of `nblocks × 8` bytes lists the
content-hash of each data block (as `__u64` values).

### Blocks (`blocks/<hash>.blk`)

Each block file starts with a 20-byte header (`struct yacfs_block_hdr`):

| Offset | Size | Field          | Description                     |
|--------|------|----------------|---------------------------------|
| 0      | 4    | magic          | `0x5A4653` ("ZFS" in ASCII)    |
| 4      | 4    | checksum       | Block content checksum          |
| 8      | 4    | orig_size      | Uncompressed data size          |
| 12     | 4    | comp_size      | Compressed data size            |
| 16     | 1    | compress       | Compression type (0 = none)     |
| 17     | 1    | checksum_type  | Checksum algorithm used         |
| 18     | 2    | pad            | Reserved                        |

The raw (or decompressed) data follows the header at offset 20.

### Directories

Directory inodes point to data blocks containing a packed sequence of directory
entries (`struct yacfs_dirent`):

| Size | Field      | Description                |
|------|------------|----------------------------|
| 2    | name_len   | Length of entry name       |
| 8    | ino        | Target inode number        |
| 1    | type       | File type (1=reg,2=dir,…)  |
| var  | name[]     | Entry name (not NUL-terminated) |

Each entry is `name_len + 11` bytes (11 = 2 + 8 + 1).

### Checksums & Compression

The inode-level `checksum_type` and `compress_type` fields specify the
algorithms used for all data blocks of that inode.  Per-block headers also
carry their own `checksum` (the computed checksum of the block payload) and
`compress`/`orig_size`/`comp_size` for transparent decompression.  The
kernel module currently passes the raw data through as-is; decompression and
checksum verification are performed by userspace tooling (`yacfs-tools`).

---

## Dependencies

- **Linux kernel headers** — for the running kernel:
  - Debian/Ubuntu: `apt install linux-headers-$(uname -r)`
  - Arch Linux: `pacman -S linux-headers`
  - RHEL/Fedora: `dnf install kernel-devel`
- **Build toolchain**: `make`, `gcc` (or `clang`), `binutils`

---

## Current Limitations

- **Read-only** — the module registers no `->write_iter`, `->create`, or
  `->unlink` operations.  Writes will return `-EROFS`.
- **No compression/decompression** — compressed blocks are exposed as their
  raw on-disk payload.  Use `yacfs-tools` to reflatten pools for direct
  kernel access.
- **No checksum verification** — block checksums are read but not verified
  in-kernel.
- **No hard link tracking** — `nlink` is populated but not enforced.
- **No extended attributes** — `xattr_block` is parsed but no `->getxattr`
  handler is wired yet.
- **No write support** — this is a *read-only* module intended for
  lightweight kernel-level access to YAcFS pools.

---

## Related Repositories

| Repository | Description |
|------------|-------------|
| [yacfs-tools](https://github.com/spivanatalie64/yacfs-tools) | Userspace tools for creating, checking, and repairing YAcFS v3 pools |
| [yacfs-metrics](https://github.com/spivanatalie64/yacfs-metrics) | Prometheus metrics exporter for YAcFS pool health |
| [yacfs-replicate](https://github.com/spivanatalie64/yacfs-replicate) | Snapshot-based replication daemon for YAcFS pools |

---

## License

This project is released under the **GNU General Public License v2.0**
(GPL-2.0).  See `LICENSE` for the full text.

---

## Authors

- **Natalie Spiva**
- **Darren Clift** — AcreetionOS
