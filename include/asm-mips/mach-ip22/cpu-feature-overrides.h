/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003 Ralf Baechle
 */
#ifndef __ASM_MACH_IP22_CPU_FEATURE_OVERRIDES_H
#define __ASM_MACH_IP22_CPU_FEATURE_OVERRIDES_H

/*
 * IP22 with a variety of processors so we can't use defaults for everything.
 */
#define cpu_has_mips16		0
#define cpu_has_divec		0
#define cpu_has_cache_cdex	1
#define cpu_has_prefetch	0
#define cpu_has_mcheck		0
#define cpu_has_ejtag		0

#define cpu_has_llsc		1
#define cpu_has_vtag_icache	0		/* Needs to change for R8000 */
#define cpu_has_dc_aliases	0
#define cpu_has_ic_fills_f_dc	0

#define cpu_has_nofpuex		0
#define cpu_has_64bits		1
#define cpu_has_64bit_addresses	1

#endif /* __ASM_MACH_IP22_CPU_FEATURE_OVERRIDES_H */
