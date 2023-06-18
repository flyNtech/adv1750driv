
char etx_array[20]="done.\n";
static int len = 1;

static struct proc_dir_entry *parent, *ido_folder, *idi_folder;

/***************** Procfs Functions *******************/
static int      open_proc(struct inode *inode, struct file *file);
static int      release_proc(struct inode *inode, struct file *file);
static ssize_t  read_proc(struct file *filp, char *buffer, size_t length,loff_t * offset);
static ssize_t  write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);

static struct proc_ops proc_fops = {
    .proc_open = open_proc,
    .proc_read = read_proc,
    .proc_write = write_proc,
    .proc_release = release_proc
};
static int open_proc(struct inode *inode, struct file *file)
{
    return 0;
}
static int release_proc(struct inode *inode, struct file *file)
{
    return 0;
}
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    char proc_name[16];
	strcpy(proc_name, filp->f_path.dentry->d_iname);
	if (StartsWith(proc_name, "set_ch"))
	{
		int value;
		if (charToInt(buff)) {
			value = 1;
		} else value = 0;
		adv_1750_gpio_set(&pci1750_gpio->chip, charToInt(proc_name+6), value);
		printk(KERN_INFO "Channel %d write %d\n", charToInt(proc_name+6),value);
	}
	if (!strcmp(proc_name, "set_all_ch"))
	{
		adv_1750_gpio_set_multiple(&pci1750_gpio->chip, (hex2int(buff)), 0);
		printk(KERN_INFO "All channels writes %X\n", hex2int(buff));
	}
    copy_from_user(etx_array,buff,len);
    return len;
}

static ssize_t read_proc(struct file *filp, char *buffer, size_t length,loff_t * offset)
{
	char proc_name[16];
	strcpy(proc_name, filp->f_path.dentry->d_iname);
	if (!strcmp(proc_name, "get_all_out"))
	{
		printk(KERN_INFO "All outputs channels (0-15): %X\n", ioread16(&pci1750_gpio->reg->rb_out0_7));
	}
	if (StartsWith(proc_name, "get_ch"))
	{
		printk(KERN_INFO "Input channel %d: %d\n", charToInt(proc_name+6),adv_1750_gpio_get(&pci1750_gpio->chip, charToInt(proc_name+6)));
	}
	if (!strcmp(proc_name, "get_all_in"))
	{
		printk(KERN_INFO "All inputs channels (0-15): %X\n", ioread16(&pci1750_gpio->reg->io_0_7));
	}
    if(len)
    {
        len=0;
    }
    else
    {
        len=1;
        return 0;
    }
    if( copy_to_user(buffer,etx_array,20) )
    {
        pr_err("Data Send : Err!\n");
    }
    return len;
}
int create_1750proc(void)
{
	parent = proc_mkdir("adv1750",NULL);
	idi_folder = proc_mkdir("IDI",parent);
	ido_folder = proc_mkdir("IDO",parent);
	proc_create("rb0-7",0666,ido_folder,&proc_fops);
	proc_create("rb8-15",0666,ido_folder,&proc_fops);
	proc_create("get_all_in",0666,idi_folder,&proc_fops);
	proc_create("get_all_out",0666,ido_folder,&proc_fops);
	proc_create("set_all_ch",0666,ido_folder,&proc_fops);
	int i = 0;
	char get_file_name[16];
	char set_file_name[16];
	while (i <= 15) {
		snprintf(get_file_name, sizeof get_file_name, "get_ch%d", i);
		proc_create(get_file_name,0666,idi_folder,&proc_fops);
		snprintf(set_file_name, sizeof set_file_name, "set_ch%d", i);
		proc_create(set_file_name,0666,ido_folder,&proc_fops);
		i++;
	}
	return 0;
}

int remove_1750proc(void)
{
	proc_remove(parent);
	return 0;
}

