/* $Id: parport.h,v 1.3 2000/03/02 02:37:13 ralf Exp $
 *
 * parport.h: ia32-specific parport initialisation
 *
 * Copyright (C) 1999  Tim Waugh <tim@cyberelk.demon.co.uk>
 *
 * This file should only be included by drivers/parport/parport_pc.c.
 */
#ifndef _ASM_PARPORT_H
#define _ASM_PARPORT_H 1

/* Maximum number of ports to support.  It is useless to set this greater
   than PARPORT_MAX (in <linux/parport.h>).  */
#define PARPORT_PC_MAX_PORTS  8

static int parport_pc_init_pci(int irq, int dma);
static int parport_pc_init_superio(void);

static int user_specified __devinitdata = 0;
int __init
parport_pc_init(int *io, int *io_hi, int *irq, int *dma)
{
	int count = 0, i = 0;

	if (io && *io) {
		/* Only probe the ports we were given. */
		user_specified = 1;
		do {
			if (!*io_hi) *io_hi = 0x400 + *io;
			if (parport_pc_probe_port(*(io++), *(io_hi++),
						  *(irq++), *(dma++), NULL))
				count++;
		} while (*io && (++i < PARPORT_PC_MAX_PORTS));
	} else {
		count += parport_pc_init_superio ();

		if (parport_pc_probe_port(0x3bc, 0x7bc, irq[0], dma[0], NULL))
			count++;
		if (parport_pc_probe_port(0x378, 0x778, irq[0], dma[0], NULL))
			count++;
		if (parport_pc_probe_port(0x278, 0x678, irq[0], dma[0], NULL))
			count++;
		
		/* probe for other PCI parallel devices */
		count += parport_pc_init_pci (irq[0], dma[0]);
	}

        return count;
}

#endif /* !(_ASM_PARPORT_H) */
