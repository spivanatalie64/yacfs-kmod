obj-m += yacfs.o
yacfs-objs := src/main.o src/super.o src/inode.o src/file.o src/dir.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules -j24

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f src/*.o src/*.o.cmd src/*.symvers src/*.order
