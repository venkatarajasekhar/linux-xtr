/*
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2000 - 2001 by Kanoj Sarcar (kanoj@sgi.com)
 * Copyright (C) 2000 - 2001 by Silicon Graphics, Inc.
 */
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/sn/arch.h>
#include <asm/sn/intr.h>
#include <asm/sn/launch.h>
#include <asm/sn/mapped_kernel.h>
#include <asm/sn/sn_private.h>
#include <asm/sn/types.h>
#include <asm/sn/sn0/hubpi.h>
#include <asm/sn/sn0/hubio.h>

/*
 * Takes as first input the PROM assigned cpu id, and the kernel
 * assigned cpu id as the second.
 */
static void alloc_cpupda(cpuid_t cpu, int cpunum)
{
	cnodeid_t node = get_cpu_cnode(cpu);
	nasid_t nasid = COMPACT_TO_NASID_NODEID(node);

	cputonasid(cpunum) = nasid;
	cputocnode(cpunum) = node;
	cputoslice(cpunum) = get_cpu_slice(cpu);
}

void __init prom_build_cpu_map(void)
{
	/* Work was already done in mlreset */
}

static void intr_clear_bits(nasid_t nasid, volatile hubreg_t *pend,
	int base_level, char *name)
{
	volatile hubreg_t bits;
	int i;

	/* Check pending interrupts */
	if ((bits = HUB_L(pend)) != 0)
		for (i = 0; i < N_INTPEND_BITS; i++)
			if (bits & (1 << i))
				LOCAL_HUB_CLR_INTR(base_level + i);
}

static void intr_clear_all(nasid_t nasid)
{
	REMOTE_HUB_S(nasid, PI_INT_MASK0_A, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK0_B, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK1_A, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK1_B, 0);
	intr_clear_bits(nasid, REMOTE_HUB_ADDR(nasid, PI_INT_PEND0),
		INT_PEND0_BASELVL, "INT_PEND0");
	intr_clear_bits(nasid, REMOTE_HUB_ADDR(nasid, PI_INT_PEND1),
		INT_PEND1_BASELVL, "INT_PEND1");
}

static void sn_mp_setup(void)
{
	cnodeid_t	cnode;
//	cpuid_t		cpu;

	for (cnode = 0; cnode < numnodes; cnode++) {
//		init_platform_nodepda();
		intr_clear_all(COMPACT_TO_NASID_NODEID(cnode));
	}
//	for (cpu = 0; cpu < maxcpus; cpu++)
//		init_platform_pda();
}

void __init prom_prepare_cpus(unsigned int max_cpus)
{
	sn_mp_setup();
	/* Master has already done per_cpu_init() */
	install_cpuintr(smp_processor_id());
#if 0
	bte_lateinit();
	ecc_init();
#endif

	replicate_kernel_text(numnodes);

	/* Done on master only */
	/* XXXKW need first == physical, second == logical */
	alloc_cpupda(cpu, 0);
}

/*
 * Launch a slave into smp_bootstrap().  It doesn't take an argument, and we
 * set sp to the kernel stack of the newly created idle process, gp to the proc
 * struct so that current_thread_info() will work.
 */
void __init prom_boot_secondary(int cpu, struct task_struct *idle)
{
	unsigned long gp = (unsigned long) idle->thread_info;
	unsigned long sp = gp + THREAD_SIZE - 32;

	LAUNCH_SLAVE(cputonasid(cpu),cputoslice(cpu),
		(launch_proc_t)MAPPED_KERN_RW_TO_K0(smp_bootstrap),
		0, (void *) sp, (void *) gp);
}

/* XXXKW implement prom_init_secondary() and prom_smp_finish to share
 * start_secondary with kernel/smp.c; otherwise, roll your own. */

void __init prom_cpus_done(void)
{
	cnodeid_t cnode;

#ifdef LATER
	Wait logic goes here.
#endif
	for (cnode = 0; cnode < numnodes; cnode++) {
#if 0
		if (cnodetocpu(cnode) == -1) {
			printk("Initializing headless hub,cnode %d", cnode);
			per_hub_init(cnode);
		}
#endif
	}
#if 0
	cpu_io_setup();
	init_mfhi_war();
#endif
}