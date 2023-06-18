obj-m += adv1750driver.o

$C_INCLUDE_PATH=/usr/include
all:
	make -C /usr/src/linux-$(shell uname -r) M=$(PWD) modules C_INCLUDE_PATH=/usr/include
clean:
	make -C /usr/src/linux-$(shell uname -r) M=$(PWD) clean C_INCLUDE_PATH=/usr/include
rnst:
	rmmod adv1750driver.ko
	make
	insmod adv1750driver.ko