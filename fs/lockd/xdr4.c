/*
 * linux/fs/lockd/xdr4.c
 *
 * XDR support for lockd and the lock client.
 *
 * Copyright (C) 1995, 1996 Olaf Kirch <okir@monad.swb.de>
 * Copyright (C) 1999, Trond Myklebust <trond.myklebust@fys.uio.no>
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/utsname.h>
#include <linux/nfs.h>

#include <linux/sunrpc/xdr.h>
#include <linux/sunrpc/clnt.h>
#include <linux/sunrpc/svc.h>
#include <linux/sunrpc/stats.h>
#include <linux/lockd/lockd.h>
#include <linux/lockd/sm_inter.h>

#define NLMDBG_FACILITY		NLMDBG_XDR
#define NLM_MAXSTRLEN		1024
#define OFFSET_MAX		((off_t)LONG_MAX)

#define QUADLEN(len)		(((len) + 3) >> 2)

u32     nlm4_deadlock, nlm4_rofs, nlm4_stale_fh, nlm4_fbig,
	nlm4_failed;


typedef struct nlm_args	nlm_args;

static inline off_t
size_to_off_t(__s64 size)
{
        size = (size > (__s64)LONG_MAX) ? (off_t)LONG_MAX : (off_t) size;
        return (size < (__s64)-LONG_MAX) ? (off_t)-LONG_MAX : (off_t) size;
}

/*
 * XDR functions for basic NLM types
 */
static u32 *
nlm4_decode_cookie(u32 *p, struct nlm_cookie *c)
{
	unsigned int	len;

	len = ntohl(*p++);
	
	if(len==0)
	{
		c->len=4;
		memset(c->data, 0, 4);	/* hockeypux brain damage */
	}
	else if(len<=8)
	{
		c->len=len;
		memcpy(c->data, p, len);
		p+=(len+3)>>2;
	}
	else 
	{
		printk(KERN_NOTICE
			"lockd: bad cookie size %d (only cookies under 8 bytes are supported.)\n", len);
		return NULL;
	}
	return p;
}

static u32 *
nlm4_encode_cookie(u32 *p, struct nlm_cookie *c)
{
	*p++ = htonl(c->len);
	memcpy(p, c->data, c->len);
	p+=(c->len+3)>>2;
	return p;
}

static u32 *
nlm4_decode_fh(u32 *p, struct nfs_fh *f)
{
	memset(f->data, 0, sizeof(f->data));
#ifdef NFS_MAXFHSIZE
	f->size = ntohl(*p++);
	if (f->size > NFS_MAXFHSIZE) {
		printk(KERN_NOTICE
			"lockd: bad fhandle size %x (should be %d)\n",
			f->size, NFS_MAXFHSIZE);
		return NULL;
	}
      	memcpy(f->data, p, f->size);
	return p + XDR_QUADLEN(f->size);
#else
	if (ntohl(*p++) != NFS_FHSIZE)
		return NULL; /* for now, all filehandles are 32 bytes */
	memcpy(f->data, p, NFS_FHSIZE);
	return p + XDR_QUADLEN(NFS_FHSIZE);
#endif
}

static u32 *
nlm4_encode_fh(u32 *p, struct nfs_fh *f)
{
#ifdef NFS_MAXFHSIZE
	*p++ = htonl(f->size);
	memcpy(p, f->data, f->size);
	return p + XDR_QUADLEN(f->size);
#else
	*p++ = htonl(NFS_FHSIZE);
	memcpy(p, f->data, NFS_FHSIZE);
	return p + XDR_QUADLEN(NFS_FHSIZE);
#endif
}

/*
 * Encode and decode owner handle
 */
static u32 *
nlm4_decode_oh(u32 *p, struct xdr_netobj *oh)
{
	return xdr_decode_netobj(p, oh);
}

static u32 *
nlm4_encode_oh(u32 *p, struct xdr_netobj *oh)
{
	return xdr_encode_netobj(p, oh);
}

static u32 *
nlm4_decode_lock(u32 *p, struct nlm_lock *lock)
{
	struct file_lock	*fl = &lock->fl;
	__s64			len, start, end;
	int			tmp;

	if (!(p = xdr_decode_string(p, &lock->caller, &tmp, NLM_MAXSTRLEN))
	 || !(p = nlm4_decode_fh(p, &lock->fh))
	 || !(p = nlm4_decode_oh(p, &lock->oh)))
		return NULL;

	memset(fl, 0, sizeof(*fl));
	fl->fl_owner = current->files;
	fl->fl_pid   = ntohl(*p++);
	fl->fl_flags = FL_POSIX;
	fl->fl_type  = F_RDLCK;		/* as good as anything else */
	p = xdr_decode_hyper(p, &start);
	p = xdr_decode_hyper(p, &len);
	end = start + len - 1;

	fl->fl_start = size_to_off_t(start);
	fl->fl_end = size_to_off_t(end);

	if (len == 0 || fl->fl_end < 0)
		fl->fl_end = OFFSET_MAX;
	return p;
}

/*
 * Encode a lock as part of an NLM call
 */
static u32 *
nlm4_encode_lock(u32 *p, struct nlm_lock *lock)
{
	struct file_lock	*fl = &lock->fl;

	if (!(p = xdr_encode_string(p, lock->caller))
	 || !(p = nlm4_encode_fh(p, &lock->fh))
	 || !(p = nlm4_encode_oh(p, &lock->oh)))
		return NULL;

	*p++ = htonl(fl->fl_pid);
	p = xdr_encode_hyper(p, fl->fl_start);
	if (fl->fl_end == OFFSET_MAX)
		p = xdr_encode_hyper(p, 0);
	else
		p = xdr_encode_hyper(p, fl->fl_end - fl->fl_start + 1);

	return p;
}

/*
 * Encode result of a TEST/TEST_MSG call
 */
static u32 *
nlm4_encode_testres(u32 *p, struct nlm_res *resp)
{
	dprintk("xdr: before encode_testres (p %p resp %p)\n", p, resp);
	if (!(p = nlm4_encode_cookie(p, &resp->cookie)))
		return 0;
	*p++ = resp->status;

	if (resp->status == nlm_lck_denied) {
		struct file_lock	*fl = &resp->lock.fl;

		*p++ = (fl->fl_type == F_RDLCK)? xdr_zero : xdr_one;
		*p++ = htonl(fl->fl_pid);

		/* Encode owner handle. */
		if (!(p = xdr_encode_netobj(p, &resp->lock.oh)))
			return 0;

		p = xdr_encode_hyper(p, fl->fl_start);
		if (fl->fl_end == OFFSET_MAX)
			p = xdr_encode_hyper(p, 0);
		else
			p = xdr_encode_hyper(p, fl->fl_end - fl->fl_start + 1);
		dprintk("xdr: encode_testres (status %d pid %d type %d start %ld end %ld)\n", resp->status, fl->fl_pid, fl->fl_type, fl->fl_start,  fl->fl_end);
	}

	dprintk("xdr: after encode_testres (p %p resp %p)\n", p, resp);
	return p;
}


/*
 * Check buffer bounds after decoding arguments
 */
static int
xdr_argsize_check(struct svc_rqst *rqstp, u32 *p)
{
	struct svc_buf	*buf = &rqstp->rq_argbuf;

	return p - buf->base <= buf->buflen;
}

static int
xdr_ressize_check(struct svc_rqst *rqstp, u32 *p)
{
	struct svc_buf	*buf = &rqstp->rq_resbuf;

	buf->len = p - buf->base;
	return (buf->len <= buf->buflen);
}

/*
 * First, the server side XDR functions
 */
int
nlm4svc_decode_testargs(struct svc_rqst *rqstp, u32 *p, nlm_args *argp)
{
	u32	exclusive;

	if (!(p = nlm4_decode_cookie(p, &argp->cookie)))
		return 0;

	exclusive = ntohl(*p++);
	if (!(p = nlm4_decode_lock(p, &argp->lock)))
		return 0;
	if (exclusive)
		argp->lock.fl.fl_type = F_WRLCK;

	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_encode_testres(struct svc_rqst *rqstp, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_encode_testres(p, resp)))
		return 0;
	return xdr_ressize_check(rqstp, p);
}

int
nlm4svc_decode_lockargs(struct svc_rqst *rqstp, u32 *p, nlm_args *argp)
{
	u32	exclusive;

	if (!(p = nlm4_decode_cookie(p, &argp->cookie)))
		return 0;
	argp->block  = ntohl(*p++);
	exclusive    = ntohl(*p++);
	if (!(p = nlm4_decode_lock(p, &argp->lock)))
		return 0;
	if (exclusive)
		argp->lock.fl.fl_type = F_WRLCK;
	argp->reclaim = ntohl(*p++);
	argp->state   = ntohl(*p++);
	argp->monitor = 1;		/* monitor client by default */

	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_decode_cancargs(struct svc_rqst *rqstp, u32 *p, nlm_args *argp)
{
	u32	exclusive;

	if (!(p = nlm4_decode_cookie(p, &argp->cookie)))
		return 0;
	argp->block = ntohl(*p++);
	exclusive = ntohl(*p++);
	if (!(p = nlm4_decode_lock(p, &argp->lock)))
		return 0;
	if (exclusive)
		argp->lock.fl.fl_type = F_WRLCK;
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_decode_unlockargs(struct svc_rqst *rqstp, u32 *p, nlm_args *argp)
{
	if (!(p = nlm4_decode_cookie(p, &argp->cookie))
	 || !(p = nlm4_decode_lock(p, &argp->lock)))
		return 0;
	argp->lock.fl.fl_type = F_UNLCK;
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_decode_shareargs(struct svc_rqst *rqstp, u32 *p, nlm_args *argp)
{
	struct nlm_lock	*lock = &argp->lock;
	int		len;

	memset(lock, 0, sizeof(*lock));
	lock->fl.fl_pid = ~(u32) 0;

	if (!(p = nlm4_decode_cookie(p, &argp->cookie))
	 || !(p = xdr_decode_string(p, &lock->caller, &len, NLM_MAXSTRLEN))
	 || !(p = nlm4_decode_fh(p, &lock->fh))
	 || !(p = nlm4_decode_oh(p, &lock->oh)))
		return 0;
	argp->fsm_mode = ntohl(*p++);
	argp->fsm_access = ntohl(*p++);
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_encode_shareres(struct svc_rqst *rqstp, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_encode_cookie(p, &resp->cookie)))
		return 0;
	*p++ = resp->status;
	*p++ = xdr_zero;		/* sequence argument */
	return xdr_ressize_check(rqstp, p);
}

int
nlm4svc_encode_res(struct svc_rqst *rqstp, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_encode_cookie(p, &resp->cookie)))
		return 0;
	*p++ = resp->status;
	return xdr_ressize_check(rqstp, p);
}

int
nlm4svc_decode_notify(struct svc_rqst *rqstp, u32 *p, struct nlm_args *argp)
{
	struct nlm_lock	*lock = &argp->lock;
	int		len;

	if (!(p = xdr_decode_string(p, &lock->caller, &len, NLM_MAXSTRLEN)))
		return 0;
	argp->state = ntohl(*p++);
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_decode_reboot(struct svc_rqst *rqstp, u32 *p, struct nlm_reboot *argp)
{
	if (!(p = xdr_decode_string(p, &argp->mon, &argp->len, SM_MAXSTRLEN)))
		return 0;
	argp->state = ntohl(*p++);
	argp->addr = ntohl(*p++);
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_decode_res(struct svc_rqst *rqstp, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_decode_cookie(p, &resp->cookie)))
		return 0;
	resp->status = ntohl(*p++);
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_decode_void(struct svc_rqst *rqstp, u32 *p, void *dummy)
{
	return xdr_argsize_check(rqstp, p);
}

int
nlm4svc_encode_void(struct svc_rqst *rqstp, u32 *p, void *dummy)
{
	return xdr_ressize_check(rqstp, p);
}

/*
 * Now, the client side XDR functions
 */
static int
nlm4clt_encode_void(struct rpc_rqst *req, u32 *p, void *ptr)
{
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_decode_void(struct rpc_rqst *req, u32 *p, void *ptr)
{
	return 0;
}

static int
nlm4clt_encode_testargs(struct rpc_rqst *req, u32 *p, nlm_args *argp)
{
	struct nlm_lock	*lock = &argp->lock;

	if (!(p = nlm4_encode_cookie(p, &argp->cookie)))
		return -EIO;
	*p++ = (lock->fl.fl_type == F_WRLCK)? xdr_one : xdr_zero;
	if (!(p = nlm4_encode_lock(p, lock)))
		return -EIO;
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_decode_testres(struct rpc_rqst *req, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_decode_cookie(p, &resp->cookie)))
		return -EIO;
	resp->status = ntohl(*p++);
	if (resp->status == NLM_LCK_DENIED) {
		struct file_lock	*fl = &resp->lock.fl;
		u32			excl;
		s64			start, end, len;

		memset(&resp->lock, 0, sizeof(resp->lock));
		excl = ntohl(*p++);
		fl->fl_pid = ntohl(*p++);
		if (!(p = nlm4_decode_oh(p, &resp->lock.oh)))
			return -EIO;

		fl->fl_flags = FL_POSIX;
		fl->fl_type  = excl? F_WRLCK : F_RDLCK;
		p = xdr_decode_hyper(p, &start);
		p = xdr_decode_hyper(p, &len);
		end = start + len - 1;

		fl->fl_start = size_to_off_t(start);
		fl->fl_end = size_to_off_t(end);
		if (len == 0 || fl->fl_end < 0)
			fl->fl_end = OFFSET_MAX;
	}
	return 0;
}


static int
nlm4clt_encode_lockargs(struct rpc_rqst *req, u32 *p, nlm_args *argp)
{
	struct nlm_lock	*lock = &argp->lock;

	if (!(p = nlm4_encode_cookie(p, &argp->cookie)))
		return -EIO;
	*p++ = argp->block? xdr_one : xdr_zero;
	*p++ = (lock->fl.fl_type == F_WRLCK)? xdr_one : xdr_zero;
	if (!(p = nlm4_encode_lock(p, lock)))
		return -EIO;
	*p++ = argp->reclaim? xdr_one : xdr_zero;
	*p++ = htonl(argp->state);
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_encode_cancargs(struct rpc_rqst *req, u32 *p, nlm_args *argp)
{
	struct nlm_lock	*lock = &argp->lock;

	if (!(p = nlm4_encode_cookie(p, &argp->cookie)))
		return -EIO;
	*p++ = argp->block? xdr_one : xdr_zero;
	*p++ = (lock->fl.fl_type == F_WRLCK)? xdr_one : xdr_zero;
	if (!(p = nlm4_encode_lock(p, lock)))
		return -EIO;
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_encode_unlockargs(struct rpc_rqst *req, u32 *p, nlm_args *argp)
{
	struct nlm_lock	*lock = &argp->lock;

	if (!(p = nlm4_encode_cookie(p, &argp->cookie)))
		return -EIO;
	if (!(p = nlm4_encode_lock(p, lock)))
		return -EIO;
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_encode_res(struct rpc_rqst *req, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_encode_cookie(p, &resp->cookie)))
		return -EIO;
	*p++ = resp->status;
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_encode_testres(struct rpc_rqst *req, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_encode_testres(p, resp)))
		return -EIO;
	req->rq_slen = xdr_adjust_iovec(req->rq_svec, p);
	return 0;
}

static int
nlm4clt_decode_res(struct rpc_rqst *req, u32 *p, struct nlm_res *resp)
{
	if (!(p = nlm4_decode_cookie(p, &resp->cookie)))
		return -EIO;
	resp->status = ntohl(*p++);
	return 0;
}

/*
 * Buffer requirements for NLM
 */
#define NLM4_void_sz		0
#define NLM4_cookie_sz		3	/* 1 len , 2 data */
#define NLM4_caller_sz		1+XDR_QUADLEN(NLM_MAXSTRLEN)
#define NLM4_netobj_sz		1+XDR_QUADLEN(XDR_MAX_NETOBJ)
/* #define NLM4_owner_sz		1+XDR_QUADLEN(NLM4_MAXOWNER) */
#define NLM4_fhandle_sz		1+XDR_QUADLEN(NFS3_FHSIZE)
#define NLM4_lock_sz		5+NLM4_caller_sz+NLM4_netobj_sz+NLM4_fhandle_sz
#define NLM4_holder_sz		6+NLM4_netobj_sz

#define NLM4_testargs_sz	NLM4_cookie_sz+1+NLM4_lock_sz
#define NLM4_lockargs_sz	NLM4_cookie_sz+4+NLM4_lock_sz
#define NLM4_cancargs_sz	NLM4_cookie_sz+2+NLM4_lock_sz
#define NLM4_unlockargs_sz	NLM4_cookie_sz+NLM4_lock_sz

#define NLM4_testres_sz		NLM4_cookie_sz+1+NLM4_holder_sz
#define NLM4_res_sz		NLM4_cookie_sz+1
#define NLM4_norep_sz		0

#ifndef MAX
# define MAX(a,b)		(((a) > (b))? (a) : (b))
#endif

/*
 * For NLM, a void procedure really returns nothing
 */
#define nlm4clt_decode_norep	NULL

#define PROC(proc, argtype, restype)				\
    { "nlm4_" #proc,						\
      (kxdrproc_t) nlm4clt_encode_##argtype,			\
      (kxdrproc_t) nlm4clt_decode_##restype,			\
      MAX(NLM4_##argtype##_sz, NLM4_##restype##_sz) << 2,	\
      0								\
    }

static struct rpc_procinfo	nlm4_procedures[] = {
    PROC(null,		void,		void),
    PROC(test,		testargs,	testres),
    PROC(lock,		lockargs,	res),
    PROC(canc,		cancargs,	res),
    PROC(unlock,	unlockargs,	res),
    PROC(granted,	testargs,	res),
    PROC(test_msg,	testargs,	norep),
    PROC(lock_msg,	lockargs,	norep),
    PROC(canc_msg,	cancargs,	norep),
    PROC(unlock_msg,	unlockargs,	norep),
    PROC(granted_msg,	testargs,	norep),
    PROC(test_res,	testres,	norep),
    PROC(lock_res,	res,		norep),
    PROC(canc_res,	res,		norep),
    PROC(unlock_res,	res,		norep),
    PROC(granted_res,	res,		norep),
    PROC(undef,		void,		void),
    PROC(undef,		void,		void),
    PROC(undef,		void,		void),
    PROC(undef,		void,		void),
#ifdef NLMCLNT_SUPPORT_SHARES
    PROC(share,		shareargs,	shareres),
    PROC(unshare,	shareargs,	shareres),
    PROC(nm_lock,	lockargs,	res),
    PROC(free_all,	notify,		void),
#else
    PROC(undef,		void,		void),
    PROC(undef,		void,		void),
    PROC(undef,		void,		void),
    PROC(undef,		void,		void),
#endif
};

struct rpc_version	nlm_version4 = {
	4, 24, nlm4_procedures,
};
