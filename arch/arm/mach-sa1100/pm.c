/*
 * SA1100 Power Management Routines
 *
 * Copyright (c) 2001 Cliff Brake <cbrake@accelent.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2001-02-06:	Cliff Brake         Initial code
 *
 * 2001-02-25:	Sukjae Cho <sjcho@east.isi.edu> &
 * 		Chester Kuo <chester@linux.org.tw>
 * 			Save more value for the resume function! Support
 * 			Bitsy/Assabet/Freebird board
 *
 * 2001-08-29:	Nicolas Pitre <nico@cam.org>
 * 			Cleaned up, pushed platform dependent stuff
 * 			in the platform specific files.
 */

/*
 * Debug macros
 */
#define DEBUG 1
#ifdef DEBUG
#  define DPRINTK(fmt, args...)	printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#  define DPRINTK(fmt, args...)
#endif


#include <linux/init.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/acpi.h>

#include <asm/hardware.h>
#include <asm/memory.h>
#include <asm/system.h>

#include "sleep.h"

extern void sa1100_cpu_suspend(void);
extern void sa1100_cpu_resume(void);

extern unsigned long *sleep_save;	/* virtual address */
extern unsigned long  sleep_save_p;	/* physical address */

#define SAVE(x)		sleep_save[SLEEP_SAVE_##x] = x
#define RESTORE(x)	x = sleep_save[SLEEP_SAVE_##x]

int pm_do_suspend(void)
{
	int retval;

	/* set up pointer to sleep parameters */
	sleep_save = kmalloc (SLEEP_SAVE_SIZE*sizeof(long), GFP_ATOMIC);
	if (!sleep_save)
		return -ENOMEM;
	sleep_save_p = virt_to_phys(sleep_save);

	retval = pm_send_all(PM_SUSPEND, (void *)2);
	if (retval) {
		kfree(sleep_save);
		return retval;
	}

	cli();

	/* preserve current time */
	RCNR = xtime.tv_sec;

	/* save vital registers */
	SAVE(OSCR);
	SAVE(OSMR0);
	SAVE(OSMR1);
	SAVE(OSMR2);
	SAVE(OSMR3);
	SAVE(OIER);

	SAVE(GPDR);
	SAVE(GRER);
	SAVE(GFER);
	SAVE(GAFR);

	SAVE(PPDR);
	SAVE(PPSR);
	SAVE(PPAR);
	SAVE(PSDR);

	SAVE(Ser1SDCR0);

	SAVE(ICMR);

	/* ... maybe a global variable initialized by arch code to set this? */
	GRER = PWER;
	GFER = 0;
	GEDR = GEDR;

	/* Clear previous reset status */
	RCSR = RCSR_HWR | RCSR_SWR | RCSR_WDR | RCSR_SMR;

	/* set resume return address */
	PSPR = virt_to_phys(sa1100_cpu_resume);

	/* go zzz */
	sa1100_cpu_suspend();

	/* ensure not to come back here if it wasn't intended */
	PSPR = 0;

	DPRINTK("*** made it back from resume\n");

	/* restore registers */
	RESTORE(GPDR);
	RESTORE(GRER);
	RESTORE(GFER);
	RESTORE(GAFR);

	/* clear any edge detect bit */
	GEDR = GEDR;

	RESTORE(PPDR);
	RESTORE(PPSR);
	RESTORE(PPAR);
	RESTORE(PSDR);

	RESTORE(Ser1SDCR0);

	PSSR = PSSR_PH;

	RESTORE(OSMR0);
	RESTORE(OSMR1);
	RESTORE(OSMR2);
	RESTORE(OSMR3);
	RESTORE(OSCR);
	RESTORE(OIER);

	ICLR = 0;
	ICCR = 1;
	RESTORE(ICMR);

	/* restore current time */
	xtime.tv_sec = RCNR;

	sti();

	kfree (sleep_save);

	retval = pm_send_all(PM_RESUME, (void *)0);
	if (retval)
		return retval;

	return 0;
}


static struct ctl_table pm_table[] =
{
	{ACPI_S1_SLP_TYP, "suspend", NULL, 0, 0600, NULL, (proc_handler *)&pm_do_suspend},
	{0}
};

static struct ctl_table pm_dir_table[] =
{
	{CTL_ACPI, "pm", NULL, 0, 0555, pm_table},
	{0}
};

/*
 * Initialize power interface
 */
static int __init pm_init(void)
{
	register_sysctl_table(pm_dir_table, 1);
	return 0;
}

__initcall(pm_init);

