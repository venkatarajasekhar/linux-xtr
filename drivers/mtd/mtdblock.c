/* 
 * Direct MTD block device access
 *
 * $Id: mtdblock.c,v 1.16 2000/06/23 09:34:58 dwmw2 Exp $
 */

#ifdef MTDBLOCK_DEBUG
#define DEBUGLVL debug
#endif							       

#include <linux/types.h>
#include <linux/module.h>

#include <linux/mtd/mtd.h>

#define MAJOR_NR MTD_BLOCK_MAJOR
#define DEVICE_NAME "mtdblock"
#define DEVICE_REQUEST mtdblock_request
#define DEVICE_NR(device) (device)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#define DEVICE_NO_RANDOM
#include <linux/blk.h>

#if LINUX_VERSION_CODE < 0x20300
#define RQFUNC_ARG void
#define blkdev_dequeue_request(req) do {CURRENT = req->next;} while (0)
#else
#define RQFUNC_ARG request_queue_t *q
#endif

#ifdef MTDBLOCK_DEBUG
static int debug = MTDBLOCK_DEBUG;
MODULE_PARM(debug, "i");
#endif

#if 1
static void mtdblock_end_request(struct request *req, int res)
{
	if (end_that_request_first( req, res, "mtdblock" ))
                return;
        end_that_request_last( req );
}
#endif

static int mtd_sizes[MAX_MTD_DEVICES];


/* Keeping a separate list rather than just getting stuff directly out of 
   the MTD core's mtd_table is perhaps not very nice, but I happen
   to dislike the idea of directly accessing mtd_table even more.
   dwmw2 31/3/0
*/

static int mtdblock_open(struct inode *inode, struct file *file)
{
	struct mtd_info *mtd = NULL;

	int dev;

	DEBUG(1,"mtdblock_open\n");
	
	if (inode == 0)
		return -EINVAL;
	
	dev = MINOR(inode->i_rdev);
	
	MOD_INC_USE_COUNT;

	mtd = get_mtd_device(NULL, dev);

	if (!mtd) {
		MOD_DEC_USE_COUNT;
		return -ENODEV;
	}

	mtd_sizes[dev] = mtd->size>>9;

	DEBUG(1, "ok\n");

	return 0;
}

static release_t mtdblock_release(struct inode *inode, struct file *file)
{
	int dev;
	struct mtd_info *mtd;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	struct super_block * sb = get_super(inode->i_rdev);
#endif
   	DEBUG(1, "mtdblock_release\n");

	if (inode == NULL)
		release_return(-ENODEV);
   
	fsync_dev(inode->i_rdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	if (sb) invalidate_inodes(sb);
#endif
	invalidate_buffers(inode->i_rdev);

	dev = MINOR(inode->i_rdev);
	mtd = __get_mtd_device(NULL, dev);

	if (!mtd) {
		printk(KERN_WARNING "MTD device is absent on mtd_release!\n");
		MOD_DEC_USE_COUNT;
		release_return(-ENODEV);
		
	}
	
	if (mtd->sync)
		mtd->sync(mtd);

	put_mtd_device(mtd);

	DEBUG(1, "ok\n");

	MOD_DEC_USE_COUNT;
	release_return(0);
}  


static void mtdblock_request(RQFUNC_ARG)
{
   struct request *current_request;
   unsigned int res = 0;
   struct mtd_info *mtd;

   while (1)
   {
      /* Grab the Request and unlink it from the request list, INIT_REQUEST
       	 will execute a return if we are done. */
      INIT_REQUEST;
      current_request = CURRENT;
   
      if (MINOR(current_request->rq_dev) >= MAX_MTD_DEVICES)
      {
	 printk("mtd: Unsupported device!\n");
	 end_request(0);
	 continue;
      }
      
      // Grab our MTD structure

      mtd = __get_mtd_device(NULL, MINOR(current_request->rq_dev));
      if (!mtd) {
	      printk("MTD device %d doesn't appear to exist any more\n", CURRENT_DEV);
	      end_request(0);
      }

      if (current_request->sector << 9 > mtd->size ||
	  (current_request->sector + current_request->nr_sectors) << 9 > mtd->size)
      {
	 printk("mtd: Attempt to read past end of device!\n");
	 printk("size: %lx, sector: %lx, nr_sectors %lx\n", mtd->size, current_request->sector, current_request->nr_sectors);
	 end_request(0);
	 continue;
      }
      
      /* Remove the request we are handling from the request list so nobody messes
         with it */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	blkdev_dequeue_request(current_request);
      
      /* Now drop the lock that the ll_rw_blk functions grabbed for us
         and process the request. This is necessary due to the extreme time
         we spend processing it. */
      spin_unlock_irq(&io_request_lock);
#endif

      // Handle the request
      switch (current_request->cmd)
      {
         size_t retlen;

	 case READ:
	 if (mtd->read(mtd,current_request->sector<<9, 
		      current_request->nr_sectors << 9, 
		      &retlen, current_request->buffer) == 0)
	    res = 1;
	 else
	    res = 0;
	 break;
	 
	 case WRITE:
//printk("mtdblock_request WRITE sector=%d(%d)\n",current_request->sector,
//	current_request->nr_sectors);

	 // Read only device
	 if ((mtd->flags & MTD_CAP_RAM) == 0)
	 {
	    res = 0;
	    break;
	 }

	 // Do the write
	 if (mtd->write(mtd,current_request->sector<<9, 
		       current_request->nr_sectors << 9, 
		       &retlen, current_request->buffer) == 0)
	    res = 1;
	 else
	    res = 0;
	 break;
	 
	 // Shouldn't happen
	 default:
	 printk("mtd: unknown request\n");
	 break;
      }

      // Grab the lock and re-thread the item onto the linked list
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	spin_lock_irq(&io_request_lock);
	mtdblock_end_request(current_request, res);
#else
	end_request(res);
#endif
   }
}



static int mtdblock_ioctl(struct inode * inode, struct file * file,
		      unsigned int cmd, unsigned long arg)
{
	struct mtd_info *mtd;

	mtd = __get_mtd_device(NULL, MINOR(inode->i_rdev));

	if (!mtd) return -EINVAL;

	switch (cmd) {
	case BLKGETSIZE:   /* Return device size */
		if (!arg)  return -EFAULT;
		return put_user((mtd->size >> 9),
                                (long *) arg);
		
	case BLKFLSBUF:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
		if(!capable(CAP_SYS_ADMIN))  return -EACCES;
#endif
		fsync_dev(inode->i_rdev);
		invalidate_buffers(inode->i_rdev);
		if (mtd->sync)
			mtd->sync(mtd);
		return 0;

	default:
		return -EINVAL;
	}
}

									/*}}}*/
#if LINUX_VERSION_CODE < 0x20326
static struct file_operations mtd_fops =
{
	open: mtdblock_open,
	ioctl: mtdblock_ioctl,
	release: mtdblock_release,
	read: block_read,
	write: block_write
};
#else
static struct block_device_operations mtd_fops = 
{
	open: mtdblock_open,
	release: mtdblock_release,
	ioctl: mtdblock_ioctl
};
#endif

#if LINUX_VERSION_CODE < 0x20300
#ifdef MODULE
#define init_mtdblock init_module
#define cleanup_mtdblock cleanup_module
#endif
#define __exit
#endif


int __init init_mtdblock(void)
{
	int i;

	if (register_blkdev(MAJOR_NR,DEVICE_NAME,&mtd_fops)) {
		printk(KERN_NOTICE "Can't allocate major number %d for Memory Technology Devices.\n",
		       MTD_BLOCK_MAJOR);
		return EAGAIN;
	}
	
	/* We fill it in at open() time. */
	for (i=0; i< MAX_MTD_DEVICES; i++) {
		mtd_sizes[i] = 0;
	}
	
	/* Allow the block size to default to BLOCK_SIZE. */
	blksize_size[MAJOR_NR] = NULL;
	blk_size[MAJOR_NR] = mtd_sizes;
	
#if LINUX_VERSION_CODE < 0x20320
	blk_dev[MAJOR_NR].request_fn = mtdblock_request;
#else
	blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), &mtdblock_request);
#endif
	return 0;
}

static void __exit cleanup_mtdblock(void)
{
	unregister_blkdev(MAJOR_NR,DEVICE_NAME);
}

#if LINUX_VERSION_CODE > 0x20300
module_init(init_mtdblock);
module_exit(cleanup_mtdblock);
#endif
