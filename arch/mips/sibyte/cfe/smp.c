/*
 * Copyright (C) 2000, 2001, 2002, 2003 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <asm/processor.h>

#include "cfe_api.h"
#include "cfe_error.h"

/*
 * Use CFE to find out how many CPUs are available, setting up
 * phys_cpu_present_map and the logical/physical mappings.
 * XXXKW will the boot CPU ever not be physical 0?
 */
void __init prom_build_cpu_map(void)
{
	int i, num;

	cpus_clear(phys_cpu_present_map);
	cpu_set(0, phys_cpu_present_map);
	__cpu_number_map[0] = 0;
	__cpu_logical_map[0] = 0;

	for (i=1, num=0; i<NR_CPUS; i++) {
		if (cfe_cpu_stop(i) == 0) {
			cpu_set(i, phys_cpu_present_map);
			__cpu_number_map[i] = ++num;
			__cpu_logical_map[num] = i;
		}
	}
	printk("Detected %i available secondary CPU(s)\n", num);
}

/*
 * Common setup before any secondaries are started
 */
void prom_prepare_cpus(void)
{
}

/*
 * Setup the PC, SP, and GP of a secondary processor and start it
 * running!
 */
int prom_boot_secondary(int cpu, struct task_struct *idle)
{
	int retval;
	extern void asmlinkage smp_bootstrap(void);
	
	retval = cfe_cpu_start(cpu_logical_map(cpu), &smp_bootstrap,
			       __KSTK_TOS(idle),
			       (unsigned long)idle->thread_info, 0);
	if (retval != 0) {
		printk("cfe_start_cpu(%i) returned %i\n" , cpu, retval);
		return 0;
	} else {
		return 1;
	}
}

/*
 * Code to run on secondary just after probing the CPU
 */
void prom_init_secondary(void)
{
	extern void sb1250_smp_init(void);
	sb1250_smp_init();
}

/*
 * Do any tidying up before marking online and running the idle
 * loop
 */
void prom_smp_finish(void)
{
	extern void sb1250_smp_finish(void);
	sb1250_smp_finish();
}

/*
 * Final cleanup after all secondaries booted
 */
void prom_cpus_done(void)
{
}
