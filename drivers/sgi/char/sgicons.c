/* $Id$
 *
 * sgicons.c: Setting up and registering console I/O on the SGI.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997 Miguel de Icaza (miguel@nuclecu.unam.mx)
 *
 * This implement a virtual console interface.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include "gconsole.h"

/* To make psaux code cleaner */
int aux_device_present = 0xaa;

/* This is the system graphics console (the first adapter found) */
struct console_ops *gconsole = 0;
struct console_ops *real_gconsole = 0;

void
enable_gconsole (void)
{
	if (!gconsole)
		gconsole = real_gconsole;
}

void
disable_gconsole (void)
{
	if (gconsole){
		real_gconsole = gconsole;
		gconsole = 0;
	}
}

void
register_gconsole (struct console_ops *gc)
{
	if (gconsole)
		return;
	gconsole = gc;
}
