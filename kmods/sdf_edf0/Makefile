mod = hgs_sdf_edf
obj-m += $(mod).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod $(mod).ko

uninstall:
	sudo rmmod $(mod)

reinstall:
	sudo rmmod $(mod)
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod $(mod).ko
