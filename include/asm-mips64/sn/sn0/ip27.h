/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Derived from IRIX <sys/SN/SN0/IP27.h>.
 *
 * Copyright (C) 1992 - 1997, 1999 Silicon Graphics, Inc.
 * Copyright (C) 1999 by Ralf Baechle
 */
#ifndef _ASM_SN_SN0_IP27_H
#define _ASM_SN_SN0_IP27_H

#include <asm/mipsregs.h>

/*
 * Simple definitions for the masks which remove SW bits from pte.
 */

#define TLBLO_HWBITSHIFT	0		/* Shift value, for masking */

#if !_LANGUAGE_ASSEMBLY

#define CAUSE_BERRINTR 		IE_IRQ5

#define ECCF_CACHE_ERR  0
#define ECCF_TAGLO      1
#define ECCF_ECC        2
#define ECCF_ERROREPC   3
#define ECCF_PADDR      4
#define ECCF_SIZE       (5 * sizeof(long))

#endif /* !_LANGUAGE_ASSEMBLY */

#if _LANGUAGE_ASSEMBLY

/*
 * KL_GET_CPUNUM (similar to EV_GET_SPNUM for EVEREST platform) reads
 * the processor number of the calling processor.  The proc parameters
 * must be a register.
 */
#define KL_GET_CPUNUM(proc) 				\
	dli	proc, LOCAL_HUB(0); 			\
	ld	proc, PI_CPU_NUM(proc)

#endif /* _LANGUAGE_ASSEMBLY */

/*
 * R10000 status register interrupt bit mask usage for IP27.
 */
#define SRB_SWTIMO	IE_SW0		/* 0x0100 */
#define SRB_NET		IE_SW1		/* 0x0200 */
#define SRB_DEV0	IE_IRQ0		/* 0x0400 */
#define SRB_DEV1	IE_IRQ1		/* 0x0800 */
#define SRB_TIMOCLK	IE_IRQ2		/* 0x1000 */
#define SRB_PROFCLK	IE_IRQ3		/* 0x2000 */
#define SRB_ERR		IE_IRQ4		/* 0x4000 */
#define SRB_SCHEDCLK	IE_IRQ5		/* 0x8000 */

#define SR_IBIT_HI	SRB_DEV0
#define SR_IBIT_PROF	SRB_PROFCLK

#define SRB_SWTIMO_IDX		0
#define SRB_NET_IDX		1
#define SRB_DEV0_IDX		2
#define SRB_DEV1_IDX		3
#define SRB_TIMOCLK_IDX		4
#define SRB_PROFCLK_IDX		5
#define SRB_ERR_IDX		6
#define SRB_SCHEDCLK_IDX	7

#define NUM_CAUSE_INTRS		8

#define SCACHE_LINESIZE	128
#define SCACHE_LINEMASK	(SCACHE_LINESIZE - 1)

#include <asm/sn/addrs.h>

#define LED_CYCLE_MASK  0x0f
#define LED_CYCLE_SHFT  4

#define SEND_NMI(_nasid, _slice)	\
          REMOTE_HUB_S((_nasid),  (PI_NMI_A + ((_slice) * PI_NMI_OFFSET)), 1)

/* Sanity hazzard ...  Below all the Origin hacks are following.  */

#define SCSI0_INT		0
#define SCSI1_INT		1
#define CPU_RESCHED_A_IRQ	3
#define CPU_RESCHED_B_IRQ	4
#define QLOGICFC_SLOT5          5
#define CPU_CALL_A_IRQ		6
#define CPU_CALL_B_IRQ		7
#define IOC3_SERIAL_INT		8
/*#define IOC3_ETH_INT		9*/
#define IOC3_ETH_INT		0x0809	/* nasid 0, wid 0x8 */

#define SN00_BRIDGE		0x9200000008000000
#define SN00I_BRIDGE0		0x920000000b000000
#define SN00I_BRIDGE1		0x920000000e000000
#define SN00I_BRIDGE2		0x920000000f000000
#endif /* _ASM_SN_SN0_IP27_H */
