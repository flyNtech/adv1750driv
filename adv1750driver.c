#include <linux/init.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/irqdesc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include "tf_func.h"
#define ADV_1750_NGPIO 32

static struct adv_1750_gpio *pci1750_gpio = NULL;


/**
 * struct adv_1750_reg
 * @out0_7:	Read: Outputs 0-7
 *		Write: Outputs 0-7
 * @in0_7:	Read: Isolated Inputs 0-7
 *		Write: Clear Interrupt
 * @irq_ctl:	Read: Enable IRQ
 *		Write: Disable IRQ
 * @filter_ctl:	Read: Activate Input Filters 0-15
 *		Write: Deactivate Input Filters 0-15
 * @out8_15:	Read: Outputs 8-15
 *		Write: Outputs 8-15
 * @in8_15:	Read: Isolated Inputs 8-15
 *		Write: Unused
 * @irq_status:	Read: Interrupt status
 *		Write: Unused
 */

/**
 * struct adv_1750_gpio - GPIO device private data structure
 * @chip:	instance of the gpio_chip
 * @lock:	synchronization lock to prevent I/O race conditions
 * @reg:	I/O address offset for the GPIO device registers
 * @irq_mask:	I/O bits affected by interrupts
 */


static struct adv_1750_reg {
	u8 io_0_7;
	u8 io_8_15;
	u8 rb_out0_7;
	u8 rb_out8_15;
	u8 boid;
	u8 reserv5;
	u8 reserv6;
	u8 reserv7;
	u8 reserv8;
	u8 reserv9;
	u8 reserv10;
	u8 reserv11;
	u8 reserv12;
	u8 reserv13;
	u8 reserv14;
	u8 reserv15;
	u8 reserv16;
	u8 reserv17;
	u8 reserv18;
	u8 reserv19;
	u8 reserv20;
	u8 reserv21;
	u8 reserv22;
	u8 reserv23;
	u8 counter0;
	u8 counter1;
	u8 counter2;
	u8 ctrl_reg;
	u8 reserv28;
	u8 reserv29;
	u8 reserv30;
	u8 reserv31;
	u8 irq_status;
};
static struct adv_1750_gpio {
	struct gpio_chip chip;
	raw_spinlock_t lock;
	struct adv_1750_reg __iomem *reg;
	unsigned long irq_mask;
};


static int adv_1750_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct adv_1750_gpio *const pci1750_gpio = gpiochip_get_data(chip);
	unsigned long mask = BIT(offset);

	if (offset < 8)
		return (ioread8(&pci1750_gpio->reg->io_0_7) >> offset) & 0x1;


	if (offset < 16)
		return (ioread8(&pci1750_gpio->reg->io_8_15) >> offset-8) & 0x1;

	if (offset < 24)
		return (ioread8(&pci1750_gpio->reg->io_0_7) & (mask >> 16));

	return (ioread8(&pci1750_gpio->reg->io_8_15) & (mask >> 24));
}

static int adv_1750_gpio_get_multiple(struct gpio_chip *chip,
	unsigned long *mask, unsigned long *bits)
{
	struct adv_1750_gpio *const pci1750_gpio = gpiochip_get_data(chip);
	unsigned long offset;
	unsigned long gpio_mask;
	void __iomem *ports[] = {
		&pci1750_gpio->reg->io_0_7, &pci1750_gpio->reg->io_8_15,
		&pci1750_gpio->reg->rb_out0_7, &pci1750_gpio->reg->rb_out8_15,
	};
	void __iomem *port_addr;
	unsigned long port_state;

	bitmap_zero(bits, chip->ngpio);

	for_each_set_clump8(offset, gpio_mask, mask, ARRAY_SIZE(ports) * 8) {
		port_addr = ports[offset / 8];
		port_state = ioread8(port_addr) & gpio_mask;

		bitmap_set_value8(bits, port_state, offset);
	}

	return 0;
}

static void adv_1750_gpio_set(struct gpio_chip *chip, unsigned int offset,
	int value)
{
	struct adv_1750_gpio *const pci1750_gpio = gpiochip_get_data(chip);
	unsigned int mask = BIT(offset);
	void __iomem *base_rd, *base_wr;
	unsigned long flags;
	unsigned int out_state;

	if (offset > 15)
		return;

	if (offset > 7) {
		mask = BIT(offset-8);
		base_rd = &pci1750_gpio->reg->rb_out8_15;
		base_wr = &pci1750_gpio->reg->io_8_15;
	}
	else {
		base_rd = &pci1750_gpio->reg->rb_out0_7;
		base_wr = &pci1750_gpio->reg->io_0_7;
	}


	if (value)
		out_state = ioread8(base_rd) | mask;
	else
		out_state = ioread8(base_rd) & ~mask;
	iowrite8(out_state, base_wr);

}

static void adv_1750_gpio_set_multiple(struct gpio_chip *chip,
	unsigned long *mask, unsigned long *bits)
{
	struct adv_1750_gpio *const pci1750_gpio = gpiochip_get_data(chip);
	void __iomem *base_rd, *base_wr;
	unsigned int out_state;
	iowrite16(mask, &pci1750_gpio->reg->io_0_7);

}


static const char *idio_16_names[ADV_1750_NGPIO] = {
	"OUT0", "OUT1", "OUT2", "OUT3", "OUT4", "OUT5", "OUT6", "OUT7",
	"OUT8", "OUT9", "OUT10", "OUT11", "OUT12", "OUT13", "OUT14", "OUT15",
	"IIN0", "IIN1", "IIN2", "IIN3", "IIN4", "IIN5", "IIN6", "IIN7",
	"IIN8", "IIN9", "IIN10", "IIN11", "IIN12", "IIN13", "IIN14", "IIN15"
};


//Enable procfs functions (probe: create_1750proc(), remove: remove_1750proc())
#include "proc1750.h"
//Enable ioctl functions (probe: create_adv1750_dev(), remove: destroy_adv1750_dev())
#include "ioctl1750.h"

static int adv1750_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *const dev = &pdev->dev;
	int err;
	const size_t pci_bar_index = 3;
	const char *const name = pci_name(pdev);
	struct gpio_irq_chip *girq;

	pci1750_gpio = devm_kzalloc(dev, sizeof(*pci1750_gpio), GFP_KERNEL);
	if (!pci1750_gpio)
		return -ENOMEM;

	err = pcim_enable_device(pdev);
	if (err) {
		dev_err(dev, "Failed to enable PCI device (%d)\n", err);
		return err;
	}

	err = pcim_iomap_regions(pdev, BIT(pci_bar_index), name);
	if (err) {
		dev_err(dev, "Unable to map PCI I/O addresses (%d)\n", err);
		return err;
	}
	pci1750_gpio->reg = pcim_iomap_table(pdev)[pci_bar_index];
	pci1750_gpio->chip.label = name;
	pci1750_gpio->chip.parent = dev;
	pci1750_gpio->chip.owner = THIS_MODULE;
	pci1750_gpio->chip.base = -1;
	pci1750_gpio->chip.ngpio = ADV_1750_NGPIO;
	pci1750_gpio->chip.names = idio_16_names;
	pci1750_gpio->chip.get = adv_1750_gpio_get;
	pci1750_gpio->chip.get_multiple = adv_1750_gpio_get_multiple;
	pci1750_gpio->chip.set = adv_1750_gpio_set;
	pci1750_gpio->chip.set_multiple = adv_1750_gpio_set_multiple;
	girq = &pci1750_gpio->chip.irq;
	girq->parent_handler = NULL;
	girq->num_parents = 0;
	girq->parents = NULL;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_edge_irq;
	raw_spin_lock_init(&pci1750_gpio->lock);

	err = devm_gpiochip_add_data(dev, &pci1750_gpio->chip, pci1750_gpio);
	if (err) {
		dev_err(dev, "GPIO registering failed (%d)\n", err);
		return err;
	}

	err = pci_enable_device_io(pdev);
	if (err) {
		dev_err(dev, "Failed to enable PCI device I/O (%d)\n", err);
		return err;
	}

	create_1750proc();
	create_adv1750_dev();

	printk(KERN_INFO "Advantech 1750 Driver loaded!\n");
	return 0;
}
static void adv1750_remove(struct pci_dev *pdev)
{
	remove_1750proc();
	destroy_adv1750_dev();
	pr_info("Advantech 1750 Driver Unloaded!");
}
static const struct pci_device_id adv_1750_device[] = {
	{ PCI_DEVICE(0x13FE, 0x1750) }, { 0 }
};
MODULE_DEVICE_TABLE(pci, adv_1750_device);

static struct pci_driver adv_1750_driver = {
	.name = "pci_1750",
	.id_table = adv_1750_device,
	.probe = adv1750_probe,
	.remove = adv1750_remove
};
module_pci_driver(adv_1750_driver);
MODULE_AUTHOR("");
MODULE_DESCRIPTION("PCI driver");
MODULE_LICENSE("GPL v2");
