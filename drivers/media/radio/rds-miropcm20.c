/*
 *  Many thanks to Fred Seidel <seidel@metabox.de>, the
 *  designer of the RDS decoder hardware. With his help
 *  I was able to code this driver.
 *  Thanks also to Norberto Pellicci, Dominic Mounteney
 *  <DMounteney@pinnaclesys.com> and www.teleauskunft.de
 *  for good hints on finding Fred. It was somewhat hard
 *  to locate him here in Germany... [:
 *
 * Revision history:
 *
 *   2000-08-09  Robert Siemer <Robert.Siemer@gmx.de>
 *        RDS support for MiroSound PCM20 radio
 */

#define _NO_VERSION_

/* #include <linux/kernel.h> */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/semaphore.h>
#include <asm/io.h>
#include "../../sound/aci.h"

#define WATCHMASK       0352 /* 11101010 */

#define DEBUG 0

static struct semaphore aci_rds_sem;


#define RDS_BUSYMASK        0x10   /* Bit 4 */
#define RDS_CLOCKMASK       0x08   /* Bit 3 */
#define RDS_DATAMASK        0x04   /* Bit 2 */


static void print_matrix(char array[], unsigned int length)
{
        int i, j;

        for (i=0; i<length; i++) {
                printk("aci-rds: ");
                for (j=7; j>=0; j--) {
                        printk("%d", (array[i] >> j) & 0x1);
                }
                if (i%8 == 0)
                        printk(" byte-border\n");
                else
                        printk("\n");
        }
}

static int byte2trans(unsigned char byte, unsigned char sendbuffer[], int size)
{
	int i;

	if (size != 8)
		return -1;
	for (i = 7; i >= 0; i--)
		sendbuffer[7-i] = (byte & (1 << i)) ? RDS_DATAMASK : 0;
	sendbuffer[0] |= RDS_CLOCKMASK;

	return 0;
}

static int trans2byte(unsigned char buffer[], int size)
{
	int i;
	unsigned char byte=0;

	if (size != 8)
		return -1;
	for (i = 7; i >= 0; i--)
		byte |= ((buffer[7-i] & RDS_DATAMASK) ? 1 : 0) << i;

	return byte;
}

static int trans2data(unsigned char readbuffer[], int readsize, unsigned char data[], int datasize)
{
	int i,j;

	if (readsize != datasize*8)
		return -1;
	for (i = 0; i < datasize; i++)
		if ((j=trans2byte(&readbuffer[i*8], 8)) < 0)
			return -1;
		else
			data[i]=j;
	return 0;
}

static int rds_waitread(void)
{
	unsigned char byte;
	int i=2000;

	do {
		byte=inb(RDS_REGISTER);
		if ((byte & WATCHMASK) != WATCHMASK)
			printk("aci-rds: Hidden information discoverd!\n");
		i--;
	}
	while ((byte & RDS_BUSYMASK) && i);

	if (i) {
#if DEBUG
		printk("rds_waitread()");
		print_matrix(&byte, 1);
#endif
		return (byte);
	} else {
		printk("aci-rds: rds_waitread() timeout...\n");
		return -1;
	}
}

/* dont use any ..._nowait() function if you are not sure what you do... */

static inline void rds_rawwrite_nowait(unsigned char byte)
{
#if DEBUG
	printk("rds_rawwrite()");
	print_matrix(&byte, 1);
#endif
	outb(byte, RDS_REGISTER);
}

static int rds_rawwrite(unsigned char byte)
{
	if (rds_waitread() >= 0) {
		rds_rawwrite_nowait(byte);
		return 0;
	} else
		return -1;
}

static int rds_write(unsigned char cmd)
{
	unsigned char sendbuffer[8];
	int i;
	
	if (byte2trans(cmd, sendbuffer, 8) != 0){
		return -1;
	} else {
		for (i=0; i<8; i++) {
			rds_rawwrite(sendbuffer[i]);
		}
	}
	return 0;
}

static int rds_readcycle_nowait(void)
{
	rds_rawwrite_nowait(0);
	return rds_waitread();
}

static int rds_readcycle(void)
{
	if (rds_rawwrite(0) < 0)
		return -1;
	return rds_waitread();
}

static int rds_read(unsigned char databuffer[], int datasize)
{

#define READSIZE (8*datasize)

	int i,j;
	unsigned char* readbuffer;

	if (!datasize)  /* nothing to read */
		return 0;

	/* to be able to use rds_readcycle_nowait()
	   I have to readwait() here */
	if (rds_waitread() < 0)
		return -1;
	
	if ((readbuffer=kmalloc(READSIZE, GFP_KERNEL)) == 0) {
		printk("aci-rds: Out of memory...\n");
		return -ENOMEM;
	} else {
		if (signal_pending(current)) {
			kfree(readbuffer);
			return -EINTR;
		}
	}
	
	for (i=0; i< READSIZE; i++)
		if((j=rds_readcycle_nowait()) < 0) {
			kfree(readbuffer);
			return -1;
		} else
			readbuffer[i]=j;
	if (trans2data(readbuffer, READSIZE, databuffer, datasize) < 0) {
		kfree(readbuffer);
		return -1;
	}
	kfree(readbuffer);
	return 0;
}

static int rds_ack(void)
{
	int i=rds_readcycle();

	if (i < 0)
		return -1;
	if (i & RDS_DATAMASK) {
		return 0;  /* ACK  */
	} else {
		return 1;  /* NACK */
	}
}

int aci_rds_cmd(unsigned char cmd, unsigned char databuffer[], int datasize)
{
	int ret;

	if (down_interruptible(&aci_rds_sem))
		return -EINTR;

	if (rds_write(cmd))
		ret = -2;

	/* RDS_RESET doesn't need further processing */
	if (cmd!=RDS_RESET && (rds_ack() || rds_read(databuffer, datasize)))
		ret = -1;
	else
		ret = 0;

	up(&aci_rds_sem);
	
	return ret;
}

int __init attach_aci_rds(void)
{
	init_MUTEX(&aci_rds_sem);
	return 0;
}

void __exit unload_aci_rds(void)
{
}
