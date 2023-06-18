#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "ioctl_1750_cmd.h"

#define MAX_DEV 1

static int adv1750_dev_open(struct inode *inode, struct file *file);
static int adv1750_dev_release(struct inode *inode, struct file *file);
static long adv1750_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static const struct file_operations mydev_fops = {
	.owner      = THIS_MODULE,
	.open       = adv1750_dev_open,
	.release    = adv1750_dev_release,
	.unlocked_ioctl = adv1750_dev_ioctl,
};

struct advantech_1750_data {
	struct device* adv1750;
	struct cdev cdev;
};

static int dev_major = 0;

static struct class *mydevclass = NULL;

static struct advantech_1750_data adv1750_data[MAX_DEV];

int create_adv1750_dev(void)
{
	int err, i;
	dev_t dev;
	err = alloc_chrdev_region(&dev, 0, MAX_DEV, "adv1750");
	dev_major = MAJOR(dev);
	mydevclass = class_create(THIS_MODULE, "adv1750");
	cdev_init(&adv1750_data[i].cdev, &mydev_fops);
	adv1750_data[i].cdev.owner = THIS_MODULE;
	cdev_add(&adv1750_data[i].cdev, MKDEV(dev_major, i), 1);
	adv1750_data[i].adv1750 = device_create(mydevclass, NULL, MKDEV(dev_major, i), NULL, "adv1750", i);
	return 0;
}

static int adv1750_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int adv1750_dev_release(struct inode *inode, struct file *file)
{
	struct my_device_private* priv = file->private_data;

	kfree(priv);

	priv = NULL;

	return 0;
}
static long adv1750_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct my_device_private* drv = file->private_data;
	int value;
	if (arg) {
		value = 1;
	} else value = 0;
	switch (cmd) {
		case READ_ALL_OUTPUT:
			printk(KERN_INFO "All outputs channels (0-15): %X\n", ioread16(&pci1750_gpio->reg->rb_out0_7));
			break;
		case READ_ALL_INPUT:
			printk(KERN_INFO "All inputs channels (0-15): %X\n", ioread16(&pci1750_gpio->reg->io_0_7));
			break;
		case READ_INPUT_CHNUM:
			printk(KERN_INFO "Input channel %d: %d\n", arg, adv_1750_gpio_get(&pci1750_gpio->chip, arg));
			break;
		case SET_ALL_OUTPUT:
			adv_1750_gpio_set_multiple(&pci1750_gpio->chip, arg, 0);
			printk(KERN_INFO "All channels writes %X\n", arg);
			break;
		case SET_OUTPUT_CHNUM_0:
			adv_1750_gpio_set(&pci1750_gpio->chip, 0, value);
			printk(KERN_INFO "Channel 0 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_1:
			adv_1750_gpio_set(&pci1750_gpio->chip, 1, value);
			printk(KERN_INFO "Channel 1 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_2:
			adv_1750_gpio_set(&pci1750_gpio->chip, 2, value);
			printk(KERN_INFO "Channel 2 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_3:
			adv_1750_gpio_set(&pci1750_gpio->chip, 3, value);
			printk(KERN_INFO "Channel 3 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_4:
			adv_1750_gpio_set(&pci1750_gpio->chip, 4, value);
			printk(KERN_INFO "Channel 4 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_5:
			adv_1750_gpio_set(&pci1750_gpio->chip, 5, value);
			printk(KERN_INFO "Channel 5 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_6:
			adv_1750_gpio_set(&pci1750_gpio->chip, 6, value);
			printk(KERN_INFO "Channel 6 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_7:
			adv_1750_gpio_set(&pci1750_gpio->chip, 7, value);
			printk(KERN_INFO "Channel 7 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_8:
			adv_1750_gpio_set(&pci1750_gpio->chip, 8, value);
			printk(KERN_INFO "Channel 8 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_9:
			adv_1750_gpio_set(&pci1750_gpio->chip, 9, value);
			printk(KERN_INFO "Channel 9 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_10:
			adv_1750_gpio_set(&pci1750_gpio->chip, 10, value);
			printk(KERN_INFO "Channel 10 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_11:
			adv_1750_gpio_set(&pci1750_gpio->chip, 11, value);
			printk(KERN_INFO "Channel 11 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_12:
			adv_1750_gpio_set(&pci1750_gpio->chip, 12, value);
			printk(KERN_INFO "Channel 12 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_13:
			adv_1750_gpio_set(&pci1750_gpio->chip, 13, value);
			printk(KERN_INFO "Channel 13 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_14:
			adv_1750_gpio_set(&pci1750_gpio->chip, 14, value);
			printk(KERN_INFO "Channel 14 write %d\n", value);
			break;
		case SET_OUTPUT_CHNUM_15:
			adv_1750_gpio_set(&pci1750_gpio->chip, 15, value);
			printk(KERN_INFO "Channel 15 write %d\n", value);
			break;


		default:
			return -EINVAL;
	};

	return 0;
}


int destroy_adv1750_dev(void)
{
	int i;

	for (i = 0; i < MAX_DEV; i++) {
		device_destroy(mydevclass, MKDEV(dev_major, i));
	}

	class_unregister(mydevclass);
	class_destroy(mydevclass);
	unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

	return 0;
}
