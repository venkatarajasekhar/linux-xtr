/*
 *  linux/arch/m68k/amiga/config.c
 *
 *  Copyright (C) 1993 Hamish Macdonald
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/*
 * Miscellaneous Amiga stuff
 */

#include <stdarg.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/kd.h>
#include <linux/tty.h>
#include <linux/console.h>
#include <linux/init.h>

#include <asm/bootinfo.h>
#include <asm/setup.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/amigahw.h>
#include <asm/amigaints.h>
#include <asm/irq.h>
#include <asm/machdep.h>
#include <linux/zorro.h>

u_long amiga_model;
u_long amiga_eclock;
u_long amiga_masterclock;
u_long amiga_colorclock;
u_long amiga_chipset;
u_char amiga_vblank;
u_char amiga_psfreq;
struct amiga_hw_present amiga_hw_present;

static const char *amiga_models[] = {
    "A500", "A500+", "A600", "A1000", "A1200", "A2000", "A2500", "A3000",
    "A3000T", "A3000+", "A4000", "A4000T", "CDTV", "CD32", "Draco"
};

extern char m68k_debug_device[];

static void amiga_sched_init(void (*handler)(int, void *, struct pt_regs *));
/* amiga specific keyboard functions */
extern int amiga_keyb_init(void);
extern int amiga_kbdrate (struct kbd_repeat *);
/* amiga specific irq functions */
extern void amiga_init_IRQ (void);
extern void (*amiga_default_handler[]) (int, void *, struct pt_regs *);
extern int amiga_request_irq (unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
                              unsigned long flags, const char *devname, void *dev_id);
extern void amiga_free_irq (unsigned int irq, void *dev_id);
extern void amiga_enable_irq (unsigned int);
extern void amiga_disable_irq (unsigned int);
static void amiga_get_model(char *model);
static int amiga_get_hardware_list(char *buffer);
extern int amiga_get_irq_list (char *);
/* amiga specific timer functions */
static unsigned long amiga_gettimeoffset (void);
static void a3000_gettod (int *, int *, int *, int *, int *, int *);
static void a2000_gettod (int *, int *, int *, int *, int *, int *);
static int amiga_hwclk (int, struct hwclk_time *);
static int amiga_set_clock_mmss (unsigned long);
extern void amiga_mksound( unsigned int count, unsigned int ticks );
#ifdef CONFIG_BLK_DEV_FD
extern int amiga_floppy_init (void);
extern void amiga_floppy_setup(char *, int *);
#endif
static void amiga_reset (void);
static void amiga_wait_key(void);
extern struct consw fb_con;
extern struct fb_info *amiga_fb_init(long *);
extern void zorro_init(void);
static void amiga_savekmsg_init(void);
static void amiga_mem_console_write(const char *b, unsigned int count);
static void amiga_serial_console_write(const char *s, unsigned int count);
static void amiga_debug_init(void);

extern void amiga_video_setup(char *, int *);

static struct console amiga_console_driver = {
    NULL, NULL, amiga_wait_key
};

extern void (*kd_mksound)(unsigned int, unsigned int);

    /*
     *  Parse an Amiga-specific record in the bootinfo
     */

int amiga_parse_bootinfo(const struct bi_record *record)
{
    int unknown = 0;
    const u_long *data = record->data;

    switch (record->tag) {
	case BI_AMIGA_MODEL:
	    amiga_model = *data;
	    break;

	case BI_AMIGA_ECLOCK:
	    amiga_eclock = *data;
	    break;

	case BI_AMIGA_CHIPSET:
	    amiga_chipset = *data;
	    break;

	case BI_AMIGA_CHIP_SIZE:
	    amiga_chip_size = *(const int *)data;
	    break;

	case BI_AMIGA_VBLANK:
	    amiga_vblank = *(const u_char *)data;
	    break;

	case BI_AMIGA_PSFREQ:
	    amiga_psfreq = *(const u_char *)data;
	    break;

	case BI_AMIGA_AUTOCON:
	    if (zorro_num_autocon < ZORRO_NUM_AUTO)
		memcpy(&zorro_autocon[zorro_num_autocon++],
		       (const struct ConfigDev *)data,
		       sizeof(struct ConfigDev));
	    else
		printk("amiga_parse_bootinfo: too many AutoConfig devices\n");
	    break;

	case BI_AMIGA_SERPER:
	    /* serial port period: ignored here */
	    break;

	default:
	    unknown = 1;
    }
    return(unknown);
}

    /*
     *  Setup the Amiga configuration info
     */

__initfunc(void config_amiga(void))
{
  /* Fill in some default values, if necessary */
  if (amiga_eclock == 0)
    amiga_eclock = 709379;

  memset(&amiga_hw_present, 0, sizeof(amiga_hw_present));

  amiga_debug_init();

  printk("Amiga hardware found: ");
  if (amiga_model >= AMI_500 && amiga_model <= AMI_DRACO)
    printk("[%s] ", amiga_models[amiga_model-AMI_500]);

  switch(amiga_model) {
  case AMI_UNKNOWN:
    goto Generic;

  case AMI_600:
  case AMI_1200:
    AMIGAHW_SET(A1200_IDE);
  case AMI_500:
  case AMI_500PLUS:
  case AMI_1000:
  case AMI_2000:
  case AMI_2500:
    AMIGAHW_SET(A2000_CLK);	/* Is this correct for all models? */
    goto Generic;

  case AMI_3000:
  case AMI_3000T:
    AMIGAHW_SET(AMBER_FF);
    AMIGAHW_SET(MAGIC_REKICK);
    /* fall through */
  case AMI_3000PLUS:
    AMIGAHW_SET(A3000_SCSI);
    AMIGAHW_SET(A3000_CLK);
    AMIGAHW_SET(ZORRO3);
    goto Generic;

  case AMI_4000T:
    AMIGAHW_SET(A4000_SCSI);
    /* fall through */
  case AMI_4000:
    AMIGAHW_SET(A4000_IDE);
    AMIGAHW_SET(A3000_CLK);
    AMIGAHW_SET(ZORRO3);
    goto Generic;

  case AMI_CDTV:
  case AMI_CD32:
    AMIGAHW_SET(CD_ROM);
    AMIGAHW_SET(A2000_CLK);             /* Is this correct? */
    goto Generic;

  Generic:
    AMIGAHW_SET(AMI_VIDEO);
    AMIGAHW_SET(AMI_BLITTER);
    AMIGAHW_SET(AMI_AUDIO);
    AMIGAHW_SET(AMI_FLOPPY);
    AMIGAHW_SET(AMI_KEYBOARD);
    AMIGAHW_SET(AMI_MOUSE);
    AMIGAHW_SET(AMI_SERIAL);
    AMIGAHW_SET(AMI_PARALLEL);
    AMIGAHW_SET(CHIP_RAM);
    AMIGAHW_SET(PAULA);

    switch(amiga_chipset) {
    case CS_OCS:
    case CS_ECS:
    case CS_AGA:
      switch (custom.deniseid & 0xf) {
      case 0x0c:
	AMIGAHW_SET(DENISE_HR);
	break;
      case 0x08:
	AMIGAHW_SET(LISA);
	break;
      }
      break;
    default:
      AMIGAHW_SET(DENISE);
      break;
    }
    switch ((custom.vposr>>8) & 0x7f) {
    case 0x00:
      AMIGAHW_SET(AGNUS_PAL);
      break;
    case 0x10:
      AMIGAHW_SET(AGNUS_NTSC);
      break;
    case 0x20:
    case 0x21:
      AMIGAHW_SET(AGNUS_HR_PAL);
      break;
    case 0x30:
    case 0x31:
      AMIGAHW_SET(AGNUS_HR_NTSC);
      break;
    case 0x22:
    case 0x23:
      AMIGAHW_SET(ALICE_PAL);
      break;
    case 0x32:
    case 0x33:
      AMIGAHW_SET(ALICE_NTSC);
      break;
    }
    AMIGAHW_SET(ZORRO);
    break;

  case AMI_DRACO:
    panic("No support for Draco yet");
 
  default:
    panic("Unknown Amiga Model");
  }

#define AMIGAHW_ANNOUNCE(name, str)			\
  if (AMIGAHW_PRESENT(name))				\
    printk(str)

  AMIGAHW_ANNOUNCE(AMI_VIDEO, "VIDEO ");
  AMIGAHW_ANNOUNCE(AMI_BLITTER, "BLITTER ");
  AMIGAHW_ANNOUNCE(AMBER_FF, "AMBER_FF ");
  AMIGAHW_ANNOUNCE(AMI_AUDIO, "AUDIO ");
  AMIGAHW_ANNOUNCE(AMI_FLOPPY, "FLOPPY ");
  AMIGAHW_ANNOUNCE(A3000_SCSI, "A3000_SCSI ");
  AMIGAHW_ANNOUNCE(A4000_SCSI, "A4000_SCSI ");
  AMIGAHW_ANNOUNCE(A1200_IDE, "A1200_IDE ");
  AMIGAHW_ANNOUNCE(A4000_IDE, "A4000_IDE ");
  AMIGAHW_ANNOUNCE(CD_ROM, "CD_ROM ");
  AMIGAHW_ANNOUNCE(AMI_KEYBOARD, "KEYBOARD ");
  AMIGAHW_ANNOUNCE(AMI_MOUSE, "MOUSE ");
  AMIGAHW_ANNOUNCE(AMI_SERIAL, "SERIAL ");
  AMIGAHW_ANNOUNCE(AMI_PARALLEL, "PARALLEL ");
  AMIGAHW_ANNOUNCE(A2000_CLK, "A2000_CLK ");
  AMIGAHW_ANNOUNCE(A3000_CLK, "A3000_CLK ");
  AMIGAHW_ANNOUNCE(CHIP_RAM, "CHIP_RAM ");
  AMIGAHW_ANNOUNCE(PAULA, "PAULA ");
  AMIGAHW_ANNOUNCE(DENISE, "DENISE ");
  AMIGAHW_ANNOUNCE(DENISE_HR, "DENISE_HR ");
  AMIGAHW_ANNOUNCE(LISA, "LISA ");
  AMIGAHW_ANNOUNCE(AGNUS_PAL, "AGNUS_PAL ");
  AMIGAHW_ANNOUNCE(AGNUS_NTSC, "AGNUS_NTSC ");
  AMIGAHW_ANNOUNCE(AGNUS_HR_PAL, "AGNUS_HR_PAL ");
  AMIGAHW_ANNOUNCE(AGNUS_HR_NTSC, "AGNUS_HR_NTSC ");
  AMIGAHW_ANNOUNCE(ALICE_PAL, "ALICE_PAL ");
  AMIGAHW_ANNOUNCE(ALICE_NTSC, "ALICE_NTSC ");
  AMIGAHW_ANNOUNCE(MAGIC_REKICK, "MAGIC_REKICK ");
  if (AMIGAHW_SET(ZORRO))
    printk("ZORRO%s ", AMIGAHW_SET(ZORRO3) ? "3" : "");
  printk("\n");

#undef AMIGAHW_ANNOUNCE

  mach_sched_init      = amiga_sched_init;
  mach_keyb_init       = amiga_keyb_init;
  mach_kbdrate         = amiga_kbdrate;
  mach_init_IRQ        = amiga_init_IRQ;
  mach_default_handler = &amiga_default_handler;
  mach_request_irq     = amiga_request_irq;
  mach_free_irq        = amiga_free_irq;
  enable_irq           = amiga_enable_irq;
  disable_irq          = amiga_disable_irq;
  mach_get_model       = amiga_get_model;
  mach_get_hardware_list = amiga_get_hardware_list;
  mach_get_irq_list    = amiga_get_irq_list;
  mach_gettimeoffset   = amiga_gettimeoffset;
  if (AMIGAHW_PRESENT(A3000_CLK)){
    mach_gettod  = a3000_gettod;
  }
  else{ /* if (AMIGAHW_PRESENT(A2000_CLK)) */
    mach_gettod  = a2000_gettod;
  }

  mach_max_dma_address = 0xffffffff; /*
				      * default MAX_DMA=0xffffffff
				      * on all machines. If we don't
				      * do so, the SCSI code will not
				      * be able to allocate any mem
				      * for transfers, unless we are
				      * dealing with a Z2 mem only
				      * system.                  /Jes
				      */

  mach_hwclk           = amiga_hwclk;
  mach_set_clock_mmss  = amiga_set_clock_mmss;
#ifdef CONFIG_BLK_DEV_FD
  mach_floppy_init     = amiga_floppy_init;
  mach_floppy_setup    = amiga_floppy_setup;
#endif
  mach_reset           = amiga_reset;
  conswitchp           = &fb_con;
  mach_fb_init         = amiga_fb_init;
  mach_video_setup     = amiga_video_setup;
  kd_mksound           = amiga_mksound;

  /* Fill in the clock values (based on the 700 kHz E-Clock) */
  amiga_masterclock = 40*amiga_eclock;	/* 28 MHz */
  amiga_colorclock = 5*amiga_eclock;	/* 3.5 MHz */

  /* clear all DMA bits */
  custom.dmacon = DMAF_ALL;
  /* ensure that the DMA master bit is set */
  custom.dmacon = DMAF_SETCLR | DMAF_MASTER;

  /* initialize chipram allocator */
  amiga_chip_init ();

  /*
   * if it is an A3000, set the magic bit that forces
   * a hard rekick
   */
  if (AMIGAHW_PRESENT(MAGIC_REKICK))
    *(u_char *)ZTWO_VADDR(0xde0002) |= 0x80;

  zorro_init();
#ifdef CONFIG_ZORRO
  /*
   * Identify all known AutoConfig Expansion Devices
   */
  zorro_identify();
#endif /* CONFIG_ZORRO */
}

static unsigned short jiffy_ticks;

__initfunc(static void amiga_sched_init(void (*timer_routine)(int, void *,
					struct pt_regs *)))
{
	jiffy_ticks = (amiga_eclock+HZ/2)/HZ;

	ciab.cra &= 0xC0;	 /* turn off timer A, continuous mode, from Eclk */
	ciab.talo = jiffy_ticks % 256;
	ciab.tahi = jiffy_ticks / 256;

	/* install interrupt service routine for CIAB Timer A
	 *
	 * Please don't change this to use ciaa, as it interferes with the
	 * SCSI code. We'll have to take a look at this later
	 */
	request_irq(IRQ_AMIGA_CIAB_TA, timer_routine, IRQ_FLG_LOCK, "timer", NULL);
	/* start timer */
	ciab.cra |= 0x11;
}

#define TICK_SIZE 10000

/* This is always executed with interrupts disabled.  */
static unsigned long amiga_gettimeoffset (void)
{
	unsigned short hi, lo, hi2;
	unsigned long ticks, offset = 0;

	/* read CIA B timer A current value */
	hi  = ciab.tahi;
	lo  = ciab.talo;
	hi2 = ciab.tahi;

	if (hi != hi2) {
		lo = ciab.talo;
		hi = hi2;
	}

	ticks = hi << 8 | lo;

	if (ticks > jiffy_ticks / 2)
		/* check for pending interrupt */
		if (cia_set_irq(&ciab_base, 0) & CIA_ICR_TA)
			offset = 10000;

	ticks = jiffy_ticks - ticks;
	ticks = (10000 * ticks) / jiffy_ticks;

	return ticks + offset;
}

static void a3000_gettod (int *yearp, int *monp, int *dayp,
			  int *hourp, int *minp, int *secp)
{
	volatile struct tod3000 *tod = TOD_3000;

	tod->cntrl1 = TOD3000_CNTRL1_HOLD;

	*secp  = tod->second1 * 10 + tod->second2;
	*minp  = tod->minute1 * 10 + tod->minute2;
	*hourp = tod->hour1   * 10 + tod->hour2;
	*dayp  = tod->day1    * 10 + tod->day2;
	*monp  = tod->month1  * 10 + tod->month2;
	*yearp = tod->year1   * 10 + tod->year2;

	tod->cntrl1 = TOD3000_CNTRL1_FREE;
}

static void a2000_gettod (int *yearp, int *monp, int *dayp,
			  int *hourp, int *minp, int *secp)
{
	volatile struct tod2000 *tod = TOD_2000;

	tod->cntrl1 = TOD2000_CNTRL1_HOLD;

	while (tod->cntrl1 & TOD2000_CNTRL1_BUSY)
		;

	*secp  = tod->second1     * 10 + tod->second2;
	*minp  = tod->minute1     * 10 + tod->minute2;
	*hourp = (tod->hour1 & 3) * 10 + tod->hour2;
	*dayp  = tod->day1        * 10 + tod->day2;
	*monp  = tod->month1      * 10 + tod->month2;
	*yearp = tod->year1       * 10 + tod->year2;

	if (!(tod->cntrl3 & TOD2000_CNTRL3_24HMODE))
		if (!(tod->hour1 & TOD2000_HOUR1_PM) && *hourp == 12)
			*hourp = 0;
		else if ((tod->hour1 & TOD2000_HOUR1_PM) && *hourp != 12)
			*hourp += 12;

	tod->cntrl1 &= ~TOD2000_CNTRL1_HOLD;
}

static int amiga_hwclk(int op, struct hwclk_time *t)
{
	if (AMIGAHW_PRESENT(A3000_CLK)) {
		volatile struct tod3000 *tod = TOD_3000;

		tod->cntrl1 = TOD3000_CNTRL1_HOLD;

		if (!op) { /* read */
			t->sec  = tod->second1 * 10 + tod->second2;
			t->min  = tod->minute1 * 10 + tod->minute2;
			t->hour = tod->hour1   * 10 + tod->hour2;
			t->day  = tod->day1    * 10 + tod->day2;
			t->wday = tod->weekday;
			t->mon  = tod->month1  * 10 + tod->month2 - 1;
			t->year = tod->year1   * 10 + tod->year2;
		} else {
			tod->second1 = t->sec / 10;
			tod->second2 = t->sec % 10;
			tod->minute1 = t->min / 10;
			tod->minute2 = t->min % 10;
			tod->hour1   = t->hour / 10;
			tod->hour2   = t->hour % 10;
			tod->day1    = t->day / 10;
			tod->day2    = t->day % 10;
			if (t->wday != -1)
				tod->weekday = t->wday;
			tod->month1  = (t->mon + 1) / 10;
			tod->month2  = (t->mon + 1) % 10;
			tod->year1   = t->year / 10;
			tod->year2   = t->year % 10;
		}

		tod->cntrl1 = TOD3000_CNTRL1_FREE;
	} else /* if (AMIGAHW_PRESENT(A2000_CLK)) */ {
		volatile struct tod2000 *tod = TOD_2000;

		tod->cntrl1 = TOD2000_CNTRL1_HOLD;
	    
		while (tod->cntrl1 & TOD2000_CNTRL1_BUSY)
			;

		if (!op) { /* read */
			t->sec  = tod->second1     * 10 + tod->second2;
			t->min  = tod->minute1     * 10 + tod->minute2;
			t->hour = (tod->hour1 & 3) * 10 + tod->hour2;
			t->day  = tod->day1        * 10 + tod->day2;
			t->wday = tod->weekday;
			t->mon  = tod->month1      * 10 + tod->month2 - 1;
			t->year = tod->year1       * 10 + tod->year2;

			if (!(tod->cntrl3 & TOD2000_CNTRL3_24HMODE))
				if (!(tod->hour1 & TOD2000_HOUR1_PM) && t->hour == 12)
					t->hour = 0;
				else if ((tod->hour1 & TOD2000_HOUR1_PM) && t->hour != 12)
					t->hour += 12;
		} else {
			tod->second1 = t->sec / 10;
			tod->second2 = t->sec % 10;
			tod->minute1 = t->min / 10;
			tod->minute2 = t->min % 10;
			if (tod->cntrl3 & TOD2000_CNTRL3_24HMODE)
				tod->hour1 = t->hour / 10;
			else if (t->hour >= 12)
				tod->hour1 = TOD2000_HOUR1_PM +
					(t->hour - 12) / 10;
			else
				tod->hour1 = t->hour / 10;
			tod->hour2   = t->hour % 10;
			tod->day1    = t->day / 10;
			tod->day2    = t->day % 10;
			if (t->wday != -1)
				tod->weekday = t->wday;
			tod->month1  = (t->mon + 1) / 10;
			tod->month2  = (t->mon + 1) % 10;
			tod->year1   = t->year / 10;
			tod->year2   = t->year % 10;
		}

		tod->cntrl1 &= ~TOD2000_CNTRL1_HOLD;
	}

	return 0;
}

static int amiga_set_clock_mmss (unsigned long nowtime)
{
	short real_seconds = nowtime % 60, real_minutes = (nowtime / 60) % 60;

	if (AMIGAHW_PRESENT(A3000_CLK)) {
		volatile struct tod3000 *tod = TOD_3000;

		tod->cntrl1 = TOD3000_CNTRL1_HOLD;

		tod->second1 = real_seconds / 10;
		tod->second2 = real_seconds % 10;
		tod->minute1 = real_minutes / 10;
		tod->minute2 = real_minutes % 10;
		
		tod->cntrl1 = TOD3000_CNTRL1_FREE;
	} else /* if (AMIGAHW_PRESENT(A2000_CLK)) */ {
		volatile struct tod2000 *tod = TOD_2000;

		tod->cntrl1 = TOD2000_CNTRL1_HOLD;
	    
		while (tod->cntrl1 & TOD2000_CNTRL1_BUSY)
			;

		tod->second1 = real_seconds / 10;
		tod->second2 = real_seconds % 10;
		tod->minute1 = real_minutes / 10;
		tod->minute2 = real_minutes % 10;

		tod->cntrl1 &= ~TOD2000_CNTRL1_HOLD;
	}

	return 0;
}

static void amiga_wait_key (void)
{
    int i;

    while (1) {
	while (ciaa.pra & 0x40);

	/* debounce */
	for (i = 0; i < 1000; i++);

	if (!(ciaa.pra & 0x40))
	    break;
    }

    /* wait for button up */
    while (1) {
	while (!(ciaa.pra & 0x40));

	/* debounce */
	for (i = 0; i < 1000; i++);

	if (ciaa.pra & 0x40)
	    break;
    }
}

void dbprintf(const char *fmt , ...)
{
	static char buf[1024];
	va_list args;
	extern void console_print (const char *str);
	extern int vsprintf(char * buf, const char * fmt, va_list args);

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	console_print (buf);
}

static NORET_TYPE void amiga_reset( void )
    ATTRIB_NORET;

static void amiga_reset (void)
{
  unsigned long jmp_addr040 = VTOP(&&jmp_addr_label040);
  unsigned long jmp_addr = VTOP(&&jmp_addr_label);

  cli();
  if (CPU_IS_040_OR_060)
    /* Setup transparent translation registers for mapping
     * of 16 MB kernel segment before disabling translation
     */
    __asm__ __volatile__
      ("movel    %0,%/d0\n\t"
       "andl     #0xff000000,%/d0\n\t"
       "orw      #0xe020,%/d0\n\t"   /* map 16 MB, enable, cacheable */
       ".chip    68040\n\t"
       "movec    %%d0,%%itt0\n\t"
       "movec    %%d0,%%dtt0\n\t"
       ".chip    68k\n\t"
       "jmp      %0@\n\t"
       : /* no outputs */
       : "a" (jmp_addr040));
  else
    /* for 680[23]0, just disable translation and jump to the physical
     * address of the label
     */
    __asm__ __volatile__
      ("pmove  %/tc,%@\n\t"
       "bclr   #7,%@\n\t"
       "pmove  %@,%/tc\n\t"
       "jmp    %0@\n\t"
       : /* no outputs */
       : "a" (jmp_addr));
 jmp_addr_label040:
  /* disable translation on '040 now */
  __asm__ __volatile__    
    ("moveq #0,%/d0\n\t"
     ".chip 68040\n\t"
     "movec %%d0,%%tc\n\t"	/* disable MMU */
     ".chip 68k\n\t"
     : /* no outputs */
     : /* no inputs */
     : "d0");

 jmp_addr_label:
  /* pickup reset address from AmigaOS ROM, reset devices and jump
   * to reset address
   */
  __asm__ __volatile__
    ("movew #0x2700,%/sr\n\t"
     "leal  0x01000000,%/a0\n\t"
     "subl  %/a0@(-0x14),%/a0\n\t"
     "movel %/a0@(4),%/a0\n\t"
     "subql #2,%/a0\n\t"
     "bra   1f\n\t"
     /* align on a longword boundary */
     __ALIGN_STR "\n"
     "1:\n\t"
     "reset\n\t"
     "jmp   %/a0@" : /* Just that gcc scans it for % escapes */ );
  
  for (;;);

}


    /*
     *  Debugging
     */

#define SAVEKMSG_MAXMEM		128*1024

#define SAVEKMSG_MAGIC1		0x53415645	/* 'SAVE' */
#define SAVEKMSG_MAGIC2		0x4B4D5347	/* 'KMSG' */

struct savekmsg {
    u_long magic1;		/* SAVEKMSG_MAGIC1 */
    u_long magic2;		/* SAVEKMSG_MAGIC2 */
    u_long magicptr;		/* address of magic1 */
    u_long size;
    char data[0];
};

static struct savekmsg *savekmsg = NULL;

static void amiga_mem_console_write(const char *s, unsigned int count)
{
    if (savekmsg->size+count <= SAVEKMSG_MAXMEM-sizeof(struct savekmsg)) {
        memcpy(savekmsg->data+savekmsg->size, s, count);
        savekmsg->size += count;
    }
}

static void amiga_savekmsg_init(void)
{
    savekmsg = (struct savekmsg *)amiga_chip_alloc(SAVEKMSG_MAXMEM);
    savekmsg->magic1 = SAVEKMSG_MAGIC1;
    savekmsg->magic2 = SAVEKMSG_MAGIC2;
    savekmsg->magicptr = VTOP(savekmsg);
    savekmsg->size = 0;
}

static void amiga_serial_putc(char c)
{
    custom.serdat = (unsigned char)c | 0x100;
    while (!(custom.serdatr & 0x2000))
	;
}

static void amiga_serial_console_write(const char *s, unsigned int count)
{
    while (count--) {
	if (*s == '\n')
	    amiga_serial_putc('\r');
	amiga_serial_putc(*s++);
    }
}

#ifdef CONFIG_SERIAL_CONSOLE
void amiga_serial_puts(const char *s)
{
    amiga_serial_console_write(s, strlen(s));
}

void amiga_serial_gets(char *s, int len)
{
    int ch, cnt = 0;

    while (1) {
	while (!(custom.intreqr & IF_RBF))
	    barrier();
	ch = custom.serdatr & 0xff;
	/* clear the interrupt, so that another character can be read */
	custom.intreq = IF_RBF;

	/* Check for backspace. */
	if (ch == 8 || ch == 127) {
	    if (cnt == 0) {
		amiga_serial_putc('\007');
		continue;
	    }
	    cnt--;
	    amiga_serial_puts("\010 \010");
	    continue;
	}

	/* Check for enter. */
	if (ch == 10 || ch == 13)
	    break;

	/* See if line is too long. */
	if (cnt >= len + 1) {
	    amiga_serial_putc(7);
	    cnt--;
	    continue;
	}

	/* Store and echo character. */
	s[cnt++] = ch;
	amiga_serial_putc(ch);
    }
    /* Print enter. */
    amiga_serial_puts("\r\n");
    s[cnt] = 0;
}
#endif

__initfunc(static void amiga_debug_init(void))
{
    if (!strcmp( m68k_debug_device, "ser" )) {
        /* no initialization required (?) */
	amiga_console_driver.write = amiga_serial_console_write;
    } else if (!strcmp( m68k_debug_device, "mem" )) {
	amiga_savekmsg_init();
	amiga_console_driver.write = amiga_mem_console_write;
    }
    register_console(&amiga_console_driver);
}


    /*
     *  Amiga specific parts of /proc
     */

static void amiga_get_model(char *model)
{
    strcpy(model, "Amiga ");
    if (amiga_model >= AMI_500 && amiga_model <= AMI_DRACO)
	strcat(model, amiga_models[amiga_model-AMI_500]);
}


static int amiga_get_hardware_list(char *buffer)
{
    int len = 0;

    if (AMIGAHW_PRESENT(CHIP_RAM))
	len += sprintf(buffer+len, "Chip RAM:\t%ldK\n", amiga_chip_size>>10);
    len += sprintf(buffer+len, "PS Freq:\t%dHz\nEClock Freq:\t%ldHz\n",
		   amiga_psfreq, amiga_eclock);
    if (AMIGAHW_PRESENT(AMI_VIDEO)) {
	char *type;
	switch(amiga_chipset) {
	    case CS_OCS:
		type = "OCS";
		break;
	    case CS_ECS:
		type = "ECS";
		break;
	    case CS_AGA:
		type = "AGA";
		break;
	    default:
		type = "Old or Unknown";
		break;
	}
	len += sprintf(buffer+len, "Graphics:\t%s\n", type);
    }

#define AMIGAHW_ANNOUNCE(name, str)			\
    if (AMIGAHW_PRESENT(name))				\
	len += sprintf (buffer+len, "\t%s\n", str)

    len += sprintf (buffer + len, "Detected hardware:\n");

    AMIGAHW_ANNOUNCE(AMI_VIDEO, "Amiga Video");
    AMIGAHW_ANNOUNCE(AMI_BLITTER, "Blitter");
    AMIGAHW_ANNOUNCE(AMBER_FF, "Amber Flicker Fixer");
    AMIGAHW_ANNOUNCE(AMI_AUDIO, "Amiga Audio");
    AMIGAHW_ANNOUNCE(AMI_FLOPPY, "Floppy Controller");
    AMIGAHW_ANNOUNCE(A3000_SCSI, "SCSI Controller WD33C93 (A3000 style)");
    AMIGAHW_ANNOUNCE(A4000_SCSI, "SCSI Controller NCR53C710 (A4000T style)");
    AMIGAHW_ANNOUNCE(A1200_IDE, "IDE Interface (A1200 style)");
    AMIGAHW_ANNOUNCE(A4000_IDE, "IDE Interface (A4000 style)");
    AMIGAHW_ANNOUNCE(CD_ROM, "Internal CD ROM drive");
    AMIGAHW_ANNOUNCE(AMI_KEYBOARD, "Keyboard");
    AMIGAHW_ANNOUNCE(AMI_MOUSE, "Mouse Port");
    AMIGAHW_ANNOUNCE(AMI_SERIAL, "Serial Port");
    AMIGAHW_ANNOUNCE(AMI_PARALLEL, "Parallel Port");
    AMIGAHW_ANNOUNCE(A2000_CLK, "Hardware Clock (A2000 style)");
    AMIGAHW_ANNOUNCE(A3000_CLK, "Hardware Clock (A3000 style)");
    AMIGAHW_ANNOUNCE(CHIP_RAM, "Chip RAM");
    AMIGAHW_ANNOUNCE(PAULA, "Paula 8364");
    AMIGAHW_ANNOUNCE(DENISE, "Denise 8362");
    AMIGAHW_ANNOUNCE(DENISE_HR, "Denise 8373");
    AMIGAHW_ANNOUNCE(LISA, "Lisa 8375");
    AMIGAHW_ANNOUNCE(AGNUS_PAL, "Normal/Fat PAL Agnus 8367/8371");
    AMIGAHW_ANNOUNCE(AGNUS_NTSC, "Normal/Fat NTSC Agnus 8361/8370");
    AMIGAHW_ANNOUNCE(AGNUS_HR_PAL, "Fat Hires PAL Agnus 8372");
    AMIGAHW_ANNOUNCE(AGNUS_HR_NTSC, "Fat Hires NTSC Agnus 8372");
    AMIGAHW_ANNOUNCE(ALICE_PAL, "PAL Alice 8374");
    AMIGAHW_ANNOUNCE(ALICE_NTSC, "NTSC Alice 8374");
    AMIGAHW_ANNOUNCE(MAGIC_REKICK, "Magic Hard Rekick");
    if (AMIGAHW_PRESENT(ZORRO))
	len += sprintf(buffer+len, "\tZorro%s AutoConfig: %d Expansion Device%s\n",
		       AMIGAHW_PRESENT(ZORRO3) ? " III" : "",
		       zorro_num_autocon, zorro_num_autocon == 1 ? "" : "s");

#undef AMIGAHW_ANNOUNCE

    return(len);
}
