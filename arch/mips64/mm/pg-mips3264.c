/*
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2002 MIPS Technologies, Inc.  All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>

#define dc_lsize	current_cpu_data.dcache.linesz

void mips3264_clear_page_dc(unsigned long page)
{
	unsigned long i;

        if (cpu_has_cache_cdex) {
	        for (i = page; i < page + PAGE_SIZE; i += dc_lsize) {
		        __asm__ __volatile__(
			        ".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				".set\tmips3\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tmips0\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_D));
		}
	}
	for (i = page; i < page + PAGE_SIZE; i += sizeof(long))
	        *(unsigned long *)(i) = 0;
}

void mips3264_copy_page_dc(unsigned long to, unsigned long from)
{
	unsigned long i;

        if (cpu_has_cache_cdex) {
	        for (i = to; i < to + PAGE_SIZE; i += dc_lsize) {
		        __asm__ __volatile__(
			        ".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				".set\tmips3\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tmips0\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_D));
		}
	}
	for (i = 0; i < PAGE_SIZE; i += sizeof(long))
	        *(unsigned long *)(to + i) = *(unsigned long *)(from + i);
}
