obj-m += mouse_md.o
obj-m += volume_md.o

# Get the current kernel version
KDIR ?= /lib/modules/$(shell uname -r)/build

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
