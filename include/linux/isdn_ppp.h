/*
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */


#ifndef _LINUX_ISDN_PPP_H
#define _LINUX_ISDN_PPP_H

#define CALLTYPE_INCOMING 0x1
#define CALLTYPE_OUTGOING 0x2
#define CALLTYPE_CALLBACK 0x4

#define IPPP_VERSION    "2.2.0"

struct pppcallinfo
{
  int calltype;
  unsigned char local_num[64];
  unsigned char remote_num[64];
  int charge_units;
};

#define PPPIOCGCALLINFO _IOWR('t',128,struct pppcallinfo)
#define PPPIOCBUNDLE   _IOW('t',129,int)
#define PPPIOCGMPFLAGS _IOR('t',130,int)
#define PPPIOCSMPFLAGS _IOW('t',131,int)
#define PPPIOCSMPMTU   _IOW('t',132,int)
#define PPPIOCSMPMRU   _IOW('t',133,int)
#define PPPIOCGCOMPRESSORS _IOR('t',134,unsigned long [8])
#define PPPIOCSCOMPRESSOR _IOW('t',135,int)
#define PPPIOCGIFNAME      _IOR('t',136, char [IFNAMSIZ] )


#define SC_MP_PROT       0x00000200
#define SC_REJ_MP_PROT   0x00000400
#define SC_OUT_SHORT_SEQ 0x00000800
#define SC_IN_SHORT_SEQ  0x00004000

#define ISDN_PPP_COMP_MAX_OPTIONS 16

#define IPPP_COMP_FLAG_XMIT 0x1
#define IPPP_COMP_FLAG_LINK 0x2

struct isdn_ppp_comp_data {
  int num;
  unsigned char options[ISDN_PPP_COMP_MAX_OPTIONS];
  int optlen;
  int flags;
};

#ifdef __KERNEL__


#include <linux/config.h>
#include <linux/skbuff.h>
#include <linux/ppp_defs.h>

#define DECOMP_ERR_NOMEM	(-10)

#define MP_END_FRAG    0x40
#define MP_BEGIN_FRAG  0x80

#define MP_MAX_QUEUE_LEN	16

/*
 * We need a way for the decompressor to influence the generation of CCP
 * Reset-Requests in a variety of ways. The decompressor is already returning
 * a lot of information (generated skb length, error conditions) so we use
 * another parameter. This parameter is a pointer to a structure which is
 * to be marked valid by the decompressor and only in this case is ever used.
 * Furthermore, the only case where this data is used is when the decom-
 * pressor returns DECOMP_ERROR.
 *
 * We use this same struct for the reset entry of the compressor to commu-
 * nicate to its caller how to deal with sending of a Reset Ack. In this
 * case, expra is not used, but other options still apply (suppressing
 * sending with rsend, appending arbitrary data, etc).
 */

#define IPPP_RESET_MAXDATABYTES	32

struct isdn_ppp_resetparams {
  unsigned char valid:1;	/* rw Is this structure filled at all ? */
  unsigned char rsend:1;	/* rw Should we send one at all ? */
  unsigned char idval:1;	/* rw Is the id field valid ? */
  unsigned char dtval:1;	/* rw Is the data field valid ? */
  unsigned char expra:1;	/* rw Is an Ack expected for this Req ? */
  unsigned char id;		/* wo Send CCP ResetReq with this id */
  unsigned short maxdlen;	/* ro Max bytes to be stored in data field */
  unsigned short dlen;		/* rw Bytes stored in data field */
  unsigned char *data;		/* wo Data for ResetReq info field */
};

/*
 * this is an 'old friend' from ppp-comp.h under a new name 
 * check the original include for more information
 */
struct isdn_ppp_compressor {
  struct isdn_ppp_compressor *next, *prev;
  int num; /* CCP compression protocol number */
  
  void *(*alloc) (struct isdn_ppp_comp_data *);
  void (*free) (void *state);
  int  (*init) (void *state, struct isdn_ppp_comp_data *,
		int unit,int debug);
  
  /* The reset entry needs to get more exact information about the
     ResetReq or ResetAck it was called with. The parameters are
     obvious. If reset is called without a Req or Ack frame which
     could be handed into it, code MUST be set to 0. Using rsparm,
     the reset entry can control if and how a ResetAck is returned. */
  
  void (*reset) (void *state, unsigned char code, unsigned char id,
		 unsigned char *data, unsigned len,
		 struct isdn_ppp_resetparams *rsparm);
  
  int  (*compress) (void *state, struct sk_buff *in,
		    struct sk_buff *skb_out, int proto);
  
	int  (*decompress) (void *state,struct sk_buff *in,
			    struct sk_buff *skb_out,
			    struct isdn_ppp_resetparams *rsparm);
  
  void (*incomp) (void *state, struct sk_buff *in,int proto);
  void (*stat) (void *state, struct compstat *stats);
};

extern int isdn_ppp_register_compressor(struct isdn_ppp_compressor *);
extern int isdn_ppp_unregister_compressor(struct isdn_ppp_compressor *);

typedef struct {
  unsigned long seqerrs;
  unsigned long frame_drops;
  unsigned long overflows;
  unsigned long max_queue_len;
} isdn_mppp_stats;

typedef struct {
  int mp_mrru;                        /* unused                             */
  struct sk_buff * frags;	/* fragments sl list -- use skb->next */
  long frames;			/* number of frames in the frame list */
  unsigned int seq;		/* last processed packet seq #: any packets
  				 * with smaller seq # will be dropped
				 * unconditionally */
  spinlock_t lock;
  int ref_ct;				 
  /* statistics */
  isdn_mppp_stats stats;
} ippp_bundle;

#define IPPP_MAX_RQ_LEN     8

/* The data structure for one CCP reset transaction */
enum ippp_ccp_reset_states {
  CCPResetIdle,
  CCPResetSentReq,
  CCPResetRcvdReq,
  CCPResetSentAck,
  CCPResetRcvdAck
};

struct ippp_ccp_reset_state {
  enum ippp_ccp_reset_states state;	/* State of this transaction */
  struct ippp_ccp *ccp;                 /* Backlink */
  unsigned char id;			/* id index */
  unsigned char ta:1;			/* The timer is active (flag) */
  unsigned char expra:1;		/* We expect a ResetAck at all */
  int dlen;				/* Databytes stored in data */
  struct timer_list timer;		/* For timeouts/retries */
  /* This is a hack but seems sufficient for the moment. We do not want
     to have this be yet another allocation for some bytes, it is more
     memory management overhead than the whole mess is worth. */
  unsigned char data[IPPP_RESET_MAXDATABYTES];
};

/* The data structure keeping track of the currently outstanding CCP Reset
   transactions. */
struct ippp_ccp_reset {
  struct ippp_ccp_reset_state *rs[256];	/* One per possible id */
  unsigned char lastid;			/* Last id allocated by the engine */
};

#endif /* __KERNEL__ */
#endif /* _LINUX_ISDN_PPP_H */
