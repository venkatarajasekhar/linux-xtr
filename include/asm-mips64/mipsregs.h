/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996, 1997, 1999 by Ralf Baechle
 * Modified for further R[236]000 support by Paul M. Antoine, 1996.
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_MIPSREGS_H
#define _ASM_MIPSREGS_H

#include <linux/linkage.h>

/*
 * The following macros are especially useful for __asm__
 * inline assembler.
 */
#ifndef __STR
#define __STR(x) #x
#endif
#ifndef STR
#define STR(x) __STR(x)
#endif

/*
 * Coprocessor 0 register names
 */
#define CP0_INDEX $0
#define CP0_RANDOM $1
#define CP0_ENTRYLO0 $2
#define CP0_ENTRYLO1 $3
#define CP0_CONTEXT $4
#define CP0_PAGEMASK $5
#define CP0_WIRED $6
#define CP0_INFO $7
#define CP0_BADVADDR $8
#define CP0_COUNT $9
#define CP0_ENTRYHI $10
#define CP0_COMPARE $11
#define CP0_STATUS $12
#define CP0_CAUSE $13
#define CP0_EPC $14
#define CP0_PRID $15
#define CP0_CONFIG $16
#define CP0_LLADDR $17
#define CP0_WATCHLO $18
#define CP0_WATCHHI $19
#define CP0_XCONTEXT $20
#define CP0_FRAMEMASK $21
#define CP0_DIAGNOSTIC $22
#define CP0_PERFORMANCE $25
#define CP0_ECC $26
#define CP0_CACHEERR $27
#define CP0_TAGLO $28
#define CP0_TAGHI $29
#define CP0_ERROREPC $30

/*
 * Coprocessor 1 (FPU) register names
 */
#define CP1_REVISION   $0
#define CP1_STATUS     $31

/*
 * Values for PageMask register
 */
#define PM_4K   0x00000000
#define PM_16K  0x00006000
#define PM_64K  0x0001e000
#define PM_256K 0x0007e000
#define PM_1M   0x001fe000
#define PM_4M   0x007fe000
#define PM_16M  0x01ffe000

/*
 * Values used for computation of new tlb entries
 */
#define PL_4K   12
#define PL_16K  14
#define PL_64K  16
#define PL_256K 18
#define PL_1M   20
#define PL_4M   22
#define PL_16M  24

/*
 * R4x00 interrupt enable / cause bits
 */
#define IE_SW0          (1<< 8)
#define IE_SW1          (1<< 9)
#define IE_IRQ0         (1<<10)
#define IE_IRQ1         (1<<11)
#define IE_IRQ2         (1<<12)
#define IE_IRQ3         (1<<13)
#define IE_IRQ4         (1<<14)
#define IE_IRQ5         (1<<15)

/*
 * R4x00 interrupt cause bits
 */
#define C_SW0           (1<< 8)
#define C_SW1           (1<< 9)
#define C_IRQ0          (1<<10)
#define C_IRQ1          (1<<11)
#define C_IRQ2          (1<<12)
#define C_IRQ3          (1<<13)
#define C_IRQ4          (1<<14)
#define C_IRQ5          (1<<15)

/*
 * Bitfields in the R4xx0 cp0 status register
 */
#define ST0_IE			0x00000001
#define ST0_EXL			0x00000002
#define ST0_ERL			0x00000004
#define ST0_KSU			0x00000018
#  define KSU_USER		0x00000010
#  define KSU_SUPERVISOR	0x00000008
#  define KSU_KERNEL		0x00000000
#define ST0_UX			0x00000020
#define ST0_SX			0x00000040
#define ST0_KX 			0x00000080
#define ST0_DE			0x00010000
#define ST0_CE			0x00020000

/*
 * Status register bits available in all MIPS CPUs.
 */
#define ST0_IM			0x0000ff00
#define  STATUSB_IP0		8
#define  STATUSF_IP0		(1   <<  8)
#define  STATUSB_IP1		9
#define  STATUSF_IP1		(1   <<  9)
#define  STATUSB_IP2		10
#define  STATUSF_IP2		(1   << 10)
#define  STATUSB_IP3		11
#define  STATUSF_IP3		(1   << 11)
#define  STATUSB_IP4		12
#define  STATUSF_IP4		(1   << 12)
#define  STATUSB_IP5		13
#define  STATUSF_IP5		(1   << 13)
#define  STATUSB_IP6		14
#define  STATUSF_IP6		(1   << 14)
#define  STATUSB_IP7		15
#define  STATUSF_IP7		(1   << 15)
#define ST0_CH			0x00040000
#define ST0_SR			0x00100000
#define ST0_TS			0x00200000
#define ST0_BEV			0x00400000
#define ST0_RE			0x02000000
#define ST0_FR			0x04000000
#define ST0_CU			0xf0000000
#define ST0_CU0			0x10000000
#define ST0_CU1			0x20000000
#define ST0_CU2			0x40000000
#define ST0_CU3			0x80000000
#define ST0_XX			0x80000000	/* MIPS IV naming */

/*
 * Bitfields and bit numbers in the coprocessor 0 cause register.
 *
 * Refer to your MIPS R4xx0 manual, chapter 5 for explanation.
 */
#define  CAUSEB_EXCCODE		2
#define  CAUSEF_EXCCODE		(31  <<  2)
#define  CAUSEB_IP		8
#define  CAUSEF_IP		(255 <<  8)
#define  CAUSEB_IP0		8
#define  CAUSEF_IP0		(1   <<  8)
#define  CAUSEB_IP1		9
#define  CAUSEF_IP1		(1   <<  9)
#define  CAUSEB_IP2		10
#define  CAUSEF_IP2		(1   << 10)
#define  CAUSEB_IP3		11
#define  CAUSEF_IP3		(1   << 11)
#define  CAUSEB_IP4		12
#define  CAUSEF_IP4		(1   << 12)
#define  CAUSEB_IP5		13
#define  CAUSEF_IP5		(1   << 13)
#define  CAUSEB_IP6		14
#define  CAUSEF_IP6		(1   << 14)
#define  CAUSEB_IP7		15
#define  CAUSEF_IP7		(1   << 15)
#define  CAUSEB_IV		23
#define  CAUSEF_IV		(1   << 23)
#define  CAUSEB_CE		28
#define  CAUSEF_CE		(3   << 28)
#define  CAUSEB_BD		31
#define  CAUSEF_BD		(1   << 31)

/*
 * Bits in the coprocessor 0 config register.
 */
#define CONF_CM_CACHABLE_NO_WA		0
#define CONF_CM_CACHABLE_WA		1
#define CONF_CM_UNCACHED		2
#define CONF_CM_CACHABLE_NONCOHERENT	3
#define CONF_CM_CACHABLE_CE		4
#define CONF_CM_CACHABLE_COW		5
#define CONF_CM_CACHABLE_CUW		6
#define CONF_CM_CACHABLE_ACCELERATED	7
#define CONF_CM_CMASK			7
#define CONF_DB				(1 <<  4)
#define CONF_IB				(1 <<  5)
#define CONF_SC				(1 << 17)

/*
 * R10000 performance counter definitions.
 *
 * FIXME: The R10000 performance counter opens a nice way to implement CPU
 *        time accounting with a precision of one cycle.  I don't have
 *        R10000 silicon but just a manual, so ...
 */

/*
 * Events counted by counter #0
 */
#define CE0_CYCLES			0
#define CE0_INSN_ISSUED			1
#define CE0_LPSC_ISSUED			2
#define CE0_S_ISSUED			3
#define CE0_SC_ISSUED			4
#define CE0_SC_FAILED			5
#define CE0_BRANCH_DECODED		6
#define CE0_QW_WB_SECONDARY		7
#define CE0_CORRECTED_ECC_ERRORS	8
#define CE0_ICACHE_MISSES		9
#define CE0_SCACHE_I_MISSES		10
#define CE0_SCACHE_I_WAY_MISSPREDICTED	11
#define CE0_EXT_INTERVENTIONS_REQ	12
#define CE0_EXT_INVALIDATE_REQ		13
#define CE0_VIRTUAL_COHERENCY_COND	14
#define CE0_INSN_GRADUATED		15

/*
 * Events counted by counter #1
 */
#define CE1_CYCLES			0
#define CE1_INSN_GRADUATED		1
#define CE1_LPSC_GRADUATED		2
#define CE1_S_GRADUATED			3
#define CE1_SC_GRADUATED		4
#define CE1_FP_INSN_GRADUATED		5
#define CE1_QW_WB_PRIMARY		6
#define CE1_TLB_REFILL			7
#define CE1_BRANCH_MISSPREDICTED	8
#define CE1_DCACHE_MISS			9
#define CE1_SCACHE_D_MISSES		10
#define CE1_SCACHE_D_WAY_MISSPREDICTED	11
#define CE1_EXT_INTERVENTION_HITS	12
#define CE1_EXT_INVALIDATE_REQ		13
#define CE1_SP_HINT_TO_CEXCL_SC_BLOCKS	14
#define CE1_SP_HINT_TO_SHARED_SC_BLOCKS	15

/*
 * These flags define in which priviledge mode the counters count events
 */
#define CEB_USER	8	/* Count events in user mode, EXL = ERL = 0 */
#define CEB_SUPERVISOR	4	/* Count events in supvervisor mode EXL = ERL = 0 */
#define CEB_KERNEL	2	/* Count events in kernel mode EXL = ERL = 0 */
#define CEB_EXL		1	/* Count events with EXL = 1, ERL = 0 */

#ifndef _LANGUAGE_ASSEMBLY
/*
 * Functions to access the performance counter and control registers
 */
extern asmlinkage unsigned int read_perf_cntr(unsigned int counter);
extern asmlinkage void write_perf_cntr(unsigned int counter, unsigned int val);
extern asmlinkage unsigned int read_perf_cntl(unsigned int counter);
extern asmlinkage void write_perf_cntl(unsigned int counter, unsigned int val);

/*
 * Macros to access the system control coprocessor
 */
#define read_32bit_cp0_register(source)                         \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
        "mfc0\t%0,"STR(source)                                  \
        : "=r" (__res));                                        \
        __res;})

#define read_64bit_cp0_register(source)                         \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
        ".set\tmips3\n\t"                                       \
        "dmfc0\t%0,"STR(source)"\n\t"                           \
        ".set\tmips0"                                           \
        : "=r" (__res));                                        \
        __res;})

#define write_32bit_cp0_register(register,value)                \
        __asm__ __volatile__(                                   \
        "mtc0\t%0,"STR(register)                                \
        : : "r" (value));

#define write_64bit_cp0_register(register,value)                \
        __asm__ __volatile__(                                   \
        ".set\tmips3\n\t"                                       \
        "dmtc0\t%0,"STR(register)"\n\t"                         \
        ".set\tmips0"                                           \
        : : "r" (value))

/* 
 * This should be changed when we get a compiler that support the MIPS32 ISA. 
 */
#define read_mips32_cp0_config1()                               \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
	".set\tnoreorder\n\t"                                   \
	".set\tnoat\n\t"                                        \
	"#.set\tmips64\n\t"					\
	"#mfc0\t$1, $16, 1\n\t"					\
	"#.set\tmips0\n\t"					\
     	".word\t0x40018001\n\t"                                 \
	"move\t%0,$1\n\t"                                       \
	".set\tat\n\t"                                          \
	".set\treorder"                                         \
	:"=r" (__res));                                         \
        __res;})

/*
 * Macros to access the floating point coprocessor control registers
 */
#define read_32bit_cp1_register(source)                         \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
	".set\tpush\n\t"					\
	".set\treorder\n\t"					\
        "cfc1\t%0,"STR(source)"\n\t"                            \
	".set\tpop"						\
        : "=r" (__res));                                        \
        __res;})

/* TLB operations. */
static inline void tlb_probe(void)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"tlbp\n\t"
		".set reorder");
}

static inline void tlb_read(void)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"tlbr\n\t"
		".set reorder");
}

static inline void tlb_write_indexed(void)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"tlbwi\n\t"
		".set reorder");
}

static inline void tlb_write_random(void)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"tlbwr\n\t"
		".set reorder");
}

/* Dealing with various CP0 mmu/cache related registers. */

/* CP0_PAGEMASK register */
static inline unsigned long get_pagemask(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"mfc0 %0, $5\n\t"
		".set reorder"
		: "=r" (val));
	return val;
}

static inline void set_pagemask(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"mtc0 %z0, $5\n\t"
		".set reorder"
		: : "Jr" (val));
}

/* CP0_ENTRYLO0 and CP0_ENTRYLO1 registers */
static inline unsigned long get_entrylo0(void)
{
	unsigned long val;

	__asm__ __volatile__(	
		".set noreorder\n\t"
		"dmfc0 %0, $2\n\t"
		".set reorder"
		: "=r" (val));
	return val;
}

static inline void set_entrylo0(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmtc0 %z0, $2\n\t"
		".set reorder"
		: : "Jr" (val));
}

static inline unsigned long get_entrylo1(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmfc0 %0, $3\n\t"
		".set reorder" : "=r" (val));

	return val;
}

static inline void set_entrylo1(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmtc0 %z0, $3\n\t"
		".set reorder"
		: : "Jr" (val));
}

/* CP0_ENTRYHI register */
static inline unsigned long get_entryhi(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmfc0 %0, $10\n\t"
		".set reorder"
		: "=r" (val));

	return val;
}

static inline void set_entryhi(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmtc0 %z0, $10\n\t"
		".set reorder"
		: : "Jr" (val));
}

/* CP0_INDEX register */
static inline unsigned int get_index(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"mfc0 %0, $0\n\t"
		".set reorder"
		: "=r" (val));
	return val;
}

static inline void set_index(unsigned int val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"mtc0 %z0, $0\n\t"
		".set reorder\n\t"
		: : "Jr" (val));
}

/* CP0_WIRED register */
static inline unsigned long get_wired(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"mfc0 %0, $6\n\t"
		".set reorder\n\t"
		: "=r" (val));
	return val;
}

static inline void set_wired(unsigned long val)
{
	__asm__ __volatile__(
		"\n\t.set noreorder\n\t"
		"mtc0 %z0, $6\n\t"
		".set reorder"
		: : "Jr" (val));
}

static inline unsigned long get_info(void)
{
	unsigned long val;

	__asm__(".set push\n\t"
		".set reorder\n\t"
		"mfc0 %0, $7\n\t"
		".set pop"
		: "=r" (val));
	return val;
}

/* CP0_STATUS registers */
static inline unsigned long get_status(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"mfc0 %0, $12\n\t"
		".set reorder"
		: "=r" (val));
	return val;
}

static inline void set_status(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"mtc0 %z0, $12\n\t"
		".set reorder"
		: : "Jr" (val));
}

/* CP0_TAGLO and CP0_TAGHI registers */
static inline unsigned long get_taglo(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"mfc0 %0, $28\n\t"
		".set reorder"
		: "=r" (val));
	return val;
}

static inline void set_taglo(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"mtc0 %z0, $28\n\t"
		".set reorder"
		: : "Jr" (val));
}

static inline unsigned long get_taghi(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"mfc0 %0, $29\n\t"
		".set reorder"
		: "=r" (val));
	return val;
}

static inline void set_taghi(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"mtc0 %z0, $29\n\t"
		".set reorder"
		: : "Jr" (val));
}

/* CP0_CONTEXT register */
static inline unsigned long get_context(void)
{
	unsigned long val;

	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmfc0 %0, $4\n\t"
		".set reorder"
		: "=r" (val));

	return val;
}

static inline void set_context(unsigned long val)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		"dmtc0 %z0, $4\n\t"
		".set reorder"
		: : "Jr" (val));
}

/*
 * Manipulate the status register.
 * Mostly used to access the interrupt bits.
 */
#define __BUILD_SET_CP0(name,register)				\
static inline unsigned int					\
set_cp0_##name(unsigned int set)				\
{								\
	unsigned int res;					\
								\
	res = read_32bit_cp0_register(register);		\
	res |= set;						\
	write_32bit_cp0_register(register, res);		\
								\
	return res;						\
}								\
								\
static inline unsigned int					\
clear_cp0_##name(unsigned int clear)				\
{								\
	unsigned int res;					\
								\
	res = read_32bit_cp0_register(register);		\
	res &= ~clear;						\
	write_32bit_cp0_register(register, res);		\
								\
	return res;						\
}								\
								\
static inline unsigned int					\
change_cp0_##name(unsigned int change, unsigned int new)	\
{								\
	unsigned int res;					\
								\
	res = read_32bit_cp0_register(register);		\
	res &= ~change;						\
	res |= (new & change);					\
	if (change)						\
		write_32bit_cp0_register(register, res);	\
								\
	return res;						\
}

__BUILD_SET_CP0(status,CP0_STATUS)
__BUILD_SET_CP0(cause,CP0_CAUSE)
__BUILD_SET_CP0(config,CP0_CONFIG)

#define __enable_fpu()							\
do {									\
	set_cp0_status(ST0_CU1);					\
	asm("nop;nop;nop;nop");		/* max. hazard */		\
} while (0)

#define __disable_fpu()							\
do {									\
	clear_cp0_status(ST0_CU1);					\
	/* We don't care about the cp0 hazard here  */			\
} while (0)

#define enable_fpu()							\
do {									\
	if (mips_cpu.options & MIPS_CPU_FPU)				\
		__enable_fpu();						\
} while (0)

#define disable_fpu()							\
do {									\
	if (mips_cpu.options & MIPS_CPU_FPU)				\
		__disable_fpu();					\
} while (0)
#endif /* defined (_LANGUAGE_ASSEMBLY) */

#endif /* _ASM_MIPSREGS_H */
