savedcmd_yacfs.mod := printf '%s\n'   src/main.o src/super.o src/inode.o src/file.o src/dir.o | awk '!x[$$0]++ { print("./"$$0) }' > yacfs.mod
