/*
 *  linux/include/asm-arm/arch-ebsa110/system.h
 *
 *  Copyright (C) 1996-2000 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

/*
 * EBSA110 idling methodology:
 *
 * We can not execute the "wait for interrupt" instruction since that
 * will stop our MCLK signal (which provides the clock for the glue
 * logic, and therefore the timer interrupt).
 *
 * Instead, we spin, polling the IRQ_STAT register for the occurrence
 * of any interrupt with core clock down to the memory clock.
 */
static void arch_idle(void)
{
	const char *irq_stat = (char *)0xff000000;
	long flags;

	if (!hlt_counter)
		return;

	do {
		/* disable interrupts */
		cli();
		/* check need_resched here to avoid races */
		if (need_resched()) {
			sti();
			return;
		}
		/* disable clock switching */
		asm volatile ("mcr%? p15, 0, ip, c15, c2, 2");
		/* wait for an interrupt to occur */
		while (!*irq_stat);
		/* enable clock switching */
		asm volatile ("mcr%? p15, 0, ip, c15, c1, 2");
		/* allow the interrupt to happen */
		sti();
	} while (!need_resched());
}

#define arch_reset(mode)	cpu_reset(0x80000000)

#endif
