/*
 *	IndyDog	0.2	A Hardware Watchdog Device for SGI IP22
 *
 *	(c) Copyright 2002 Guido Guenther <agx@sigxcpu.org>, All Rights Reserved.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *	
 *	based on softdog.c by Alan Cox <alan@redhat.com>
 */
 
#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/smp_lock.h>
#include <asm/uaccess.h>
#include <asm/sgi/mc.h>

static int indydog_alive;

#ifdef CONFIG_WATCHDOG_NOWAYOUT
static int nowayout = 1;
#else
static int nowayout = 0;
#endif

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default=CONFIG_WATCHDOG_NOWAYOUT)");

static inline void indydog_ping(void)
{
	sgimc->watchdogt = 0;
}

/*
 *	Allow only one person to hold it open
 */
static int indydog_open(struct inode *inode, struct file *file)
{
	u32 mc_ctrl0;
	
	if(indydog_alive)
		return -EBUSY;

	if (nowayout)
		__module_get(THIS_MODULE);

	/* Activate timer */
	mc_ctrl0 = sgimc->cpuctrl0 | SGIMC_CCTRL0_WDOG;
	sgimc->cpuctrl0 = mc_ctrl0;
	indydog_ping();
			
	indydog_alive = 1;
	printk(KERN_INFO "Started watchdog timer.\n");
	
	return 0;
}

static int indydog_release(struct inode *inode, struct file *file)
{
	/* Shut off the timer.
	 * Lock it in if it's a module and we defined ...NOWAYOUT */
	lock_kernel();
	if (!nowayout) {
		u32 mc_ctrl0 = sgimc->cpuctrl0; 
		mc_ctrl0 &= ~SGIMC_CCTRL0_WDOG;
		sgimc->cpuctrl0 = mc_ctrl0;
		printk(KERN_INFO "Stopped watchdog timer.\n");
	}
	indydog_alive = 0;
	unlock_kernel();
	
	return 0;
}

static ssize_t indydog_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
	/* Can't seek (pwrite) on this device */
	if (ppos != &file->f_pos)
		return -ESPIPE;

	/* Refresh the timer. */
	if (len) {
		indydog_ping();
	}
	return len;
}

static int indydog_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	static struct watchdog_info ident = {
		identity: "Hardware Watchdog for SGI IP22",
	};
	switch (cmd) {
		default:
			return -ENOIOCTLCMD;
		case WDIOC_GETSUPPORT:
			if(copy_to_user((struct watchdog_info *)arg, &ident, sizeof(ident)))
				return -EFAULT;
			return 0;
		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			return put_user(0,(int *)arg);
		case WDIOC_KEEPALIVE:
			indydog_ping();
			return 0;
	}
}

static struct file_operations indydog_fops = {
	owner:		THIS_MODULE,
	write:		indydog_write,
	ioctl:		indydog_ioctl,
	open:		indydog_open,
	release:	indydog_release,
};

static struct miscdevice indydog_miscdev = {
	minor:		WATCHDOG_MINOR,
	name:		"watchdog",
	fops:		&indydog_fops,
};

static char banner[] __initdata = KERN_INFO "Hardware Watchdog Timer for SGI IP22: 0.2\n";

static int __init watchdog_init(void)
{
	int ret = misc_register(&indydog_miscdev);

	if (ret)
		return ret;

	printk(banner);

	return 0;
}

static void __exit watchdog_exit(void)
{
	misc_deregister(&indydog_miscdev);
}

module_init(watchdog_init);
module_exit(watchdog_exit);
MODULE_LICENSE("GPL");
