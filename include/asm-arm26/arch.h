/*
 *  linux/include/asm-arm/mach/arch.h
 *
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The size of struct machine_desc
 *   (for assembler code)
 * FIXME - I count 45... or is this padding?
 */
#define SIZEOF_MACHINE_DESC	48

#ifndef __ASSEMBLY__

struct tag;

struct machine_desc {
	int			nr; /* arch no FIXME - get rid */
	const char		*name;		/* architecture name	*/
	unsigned int		param_offset;	/* parameter page	*/

	unsigned int		video_start;	/* start of video RAM	*/
	unsigned int		video_end;	/* end of video RAM	*/

	unsigned int		reserve_lp0 :1;	/* never has lp0	*/
	unsigned int		reserve_lp1 :1;	/* never has lp1	*/
	unsigned int		reserve_lp2 :1;	/* never has lp2	*/
	unsigned int		soft_reboot :1;	/* soft reboot		*/
	void			(*fixup)(struct machine_desc *,
					 struct tag *, char **,
					 struct meminfo *);
	void			(*map_io)(void);/* IO mapping function	*/
	void			(*init_irq)(void);
};

/*
 * Set of macros to define architecture features.  This is built into
 * a table by the linker.
 */
#define MACHINE_START(_type,_name)		\
const struct machine_desc __mach_desc_##_type	\
 __attribute__((__section__(".arch.info"))) = {	\
	nr:		MACH_TYPE_##_type,	\
	name:		_name,

#define MAINTAINER(n)

#define BOOT_PARAMS(_params)			\
	param_offset:	_params,

#define INITIRQ(_func)				\
	init_irq:	_func,

#define MACHINE_END				\
};

#endif