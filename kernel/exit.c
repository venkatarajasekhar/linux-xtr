/*
 *  linux/kernel/exit.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/config.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/resource.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/malloc.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/mmu_context.h>

extern void sem_exit (void);
extern void acct_process (long exitcode);
extern void kerneld_exit(void);

int getrusage(struct task_struct *, int, struct rusage *);

static inline void generate(unsigned long sig, struct task_struct * p)
{
	unsigned flags;
	unsigned long mask = 1 << (sig-1);
	struct sigaction * sa = sig + p->sig->action - 1;

	/*
	 * Optimize away the signal, if it's a signal that can
	 * be handled immediately (ie non-blocked and untraced)
	 * and that is ignored (either explicitly or by default)
	 */
	spin_lock_irqsave(&p->sig->siglock, flags);
	if (!(mask & p->blocked) && !(p->flags & PF_PTRACED)) {
		/* don't bother with ignored signals (but SIGCHLD is special) */
		if (sa->sa_handler == SIG_IGN && sig != SIGCHLD)
			goto out;
		/* some signals are ignored by default.. (but SIGCONT already did its deed) */
		if ((sa->sa_handler == SIG_DFL) &&
		    (sig == SIGCONT || sig == SIGCHLD || sig == SIGWINCH || sig == SIGURG))
			goto out;
	}
	spin_lock(&p->sigmask_lock);
	p->signal |= mask;
	spin_unlock(&p->sigmask_lock);
	if (p->state == TASK_INTERRUPTIBLE && signal_pending(p))
		wake_up_process(p);
out:
	spin_unlock_irqrestore(&p->sig->siglock, flags);
}

/*
 * Force a signal that the process can't ignore: if necessary
 * we unblock the signal and change any SIG_IGN to SIG_DFL.
 */
void force_sig(unsigned long sig, struct task_struct * p)
{
	sig--;
	if (p->sig) {
		unsigned flags;
		unsigned long mask = 1UL << sig;
		struct sigaction *sa = p->sig->action + sig;

		spin_lock_irqsave(&p->sig->siglock, flags);

		spin_lock(&p->sigmask_lock);
		p->signal |= mask;
		p->blocked &= ~mask;
		spin_unlock(&p->sigmask_lock);

		if (sa->sa_handler == SIG_IGN)
			sa->sa_handler = SIG_DFL;
		if (p->state == TASK_INTERRUPTIBLE)
			wake_up_process(p);

		spin_unlock_irqrestore(&p->sig->siglock, flags);
	}
}

int send_sig(unsigned long sig,struct task_struct * p,int priv)
{
	if (!p || sig > 32)
		return -EINVAL;
	if (!priv && ((sig != SIGCONT) || (current->session != p->session)) &&
	    (current->euid ^ p->suid) && (current->euid ^ p->uid) &&
	    (current->uid ^ p->suid) && (current->uid ^ p->uid) &&
	    !suser())
		return -EPERM;

	if (sig && p->sig) {
		unsigned flags;
		spin_lock_irqsave(&p->sigmask_lock, flags);
		if ((sig == SIGKILL) || (sig == SIGCONT)) {
			if (p->state == TASK_STOPPED)
				wake_up_process(p);
			p->exit_code = 0;
			p->signal &= ~( (1<<(SIGSTOP-1)) | (1<<(SIGTSTP-1)) |
					(1<<(SIGTTIN-1)) | (1<<(SIGTTOU-1)) );
		}
		if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU)
			p->signal &= ~(1<<(SIGCONT-1));
		spin_unlock_irqrestore(&p->sigmask_lock, flags);

		/* Actually generate the signal */
		generate(sig,p);
	}
	return 0;
}

void notify_parent(struct task_struct * tsk, int signal)
{
	struct task_struct * parent = tsk->p_pptr;

	send_sig(signal, parent, 1);
	wake_up_interruptible(&parent->wait_chldexit);
}

void release(struct task_struct * p)
{
	if (p != current) {
#ifdef __SMP__
		/* FIXME! Cheesy, but kills the window... -DaveM */
		do {
			barrier();
		} while (p->has_cpu);
		spin_unlock_wait(&scheduler_lock);
#endif
		charge_uid(p, -1);
		nr_tasks--;
		add_free_taskslot(p->tarray_ptr);
		unhash_pid(p);
		REMOVE_LINKS(p);
		release_thread(p);
		current->cmin_flt += p->min_flt + p->cmin_flt;
		current->cmaj_flt += p->maj_flt + p->cmaj_flt;
		current->cnswap += p->nswap + p->cnswap;
		free_task_struct(p);
	} else {
		printk("task releasing itself\n");
	}
}

/*
 * This checks not only the pgrp, but falls back on the pid if no
 * satisfactory pgrp is found. I dunno - gdb doesn't work correctly
 * without this...
 */
int session_of_pgrp(int pgrp)
{
	struct task_struct *p;
	int fallback;

	fallback = -1;
	read_lock(&tasklist_lock);
	for_each_task(p) {
 		if (p->session <= 0)
 			continue;
		if (p->pgrp == pgrp) {
			fallback = p->session;
			break;
		}
		if (p->pid == pgrp)
			fallback = p->session;
	}
	read_unlock(&tasklist_lock);
	return fallback;
}

/*
 * kill_pg() sends a signal to a process group: this is what the tty
 * control characters do (^C, ^Z etc)
 */
int kill_pg(int pgrp, int sig, int priv)
{
	int retval;

	retval = -EINVAL;
	if (sig >= 0 && sig <= 32 && pgrp > 0) {
		struct task_struct *p;
		int found = 0;

		retval = -ESRCH;
		read_lock(&tasklist_lock);
		for_each_task(p) {
			if (p->pgrp == pgrp) {
				int err = send_sig(sig,p,priv);
				if (err != 0)
					retval = err;
				else
					found++;
			}
		}
		read_unlock(&tasklist_lock);
		if (found)
			retval = 0;
	}
	return retval;
}

/*
 * kill_sl() sends a signal to the session leader: this is used
 * to send SIGHUP to the controlling process of a terminal when
 * the connection is lost.
 */
int kill_sl(int sess, int sig, int priv)
{
	int retval;

	retval = -EINVAL;
	if (sig >= 0 && sig <= 32 && sess > 0) {
		struct task_struct *p;
		int found = 0;

		retval = -ESRCH;
		read_lock(&tasklist_lock);
		for_each_task(p) {
			if (p->leader && p->session == sess) {
				int err = send_sig(sig,p,priv);

				if (err)
					retval = err;
				else
					found++;
			}
		}
		read_unlock(&tasklist_lock);
		if (found)
			retval = 0;
	}
	return retval;
}

int kill_proc(int pid, int sig, int priv)
{
	int retval;

	retval = -EINVAL;
	if (sig >= 0 && sig <= 32) {
		struct task_struct *p = find_task_by_pid(pid);
		
		if(p)
			retval = send_sig(sig, p, priv);
		else
			retval = -ESRCH;
	}
	return retval;
}

/*
 * POSIX specifies that kill(-1,sig) is unspecified, but what we have
 * is probably wrong.  Should make it like BSD or SYSV.
 */
asmlinkage int sys_kill(int pid,int sig)
{
	if (!pid)
		return kill_pg(current->pgrp,sig,0);

	if (pid == -1) {
		int retval = 0, count = 0;
		struct task_struct * p;

		read_lock(&tasklist_lock);
		for_each_task(p) {
			if (p->pid > 1 && p != current) {
				int err;
				++count;
				if ((err = send_sig(sig,p,0)) != -EPERM)
					retval = err;
			}
		}
		read_unlock(&tasklist_lock);
		return count ? retval : -ESRCH;
	}
	if (pid < 0)
		return kill_pg(-pid,sig,0);

	/* Normal kill */
	return kill_proc(pid,sig,0);
}

/*
 * Determine if a process group is "orphaned", according to the POSIX
 * definition in 2.2.2.52.  Orphaned process groups are not to be affected
 * by terminal-generated stop signals.  Newly orphaned process groups are
 * to receive a SIGHUP and a SIGCONT.
 *
 * "I ask you, have you ever known what it is to be an orphan?"
 */
static int will_become_orphaned_pgrp(int pgrp, struct task_struct * ignored_task)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_task(p) {
		if ((p == ignored_task) || (p->pgrp != pgrp) ||
		    (p->state == TASK_ZOMBIE) ||
		    (p->p_pptr->pid == 1))
			continue;
		if ((p->p_pptr->pgrp != pgrp) &&
		    (p->p_pptr->session == p->session)) {
			read_unlock(&tasklist_lock);
 			return 0;
		}
	}
	read_unlock(&tasklist_lock);
	return 1;	/* (sighing) "Often!" */
}

int is_orphaned_pgrp(int pgrp)
{
	return will_become_orphaned_pgrp(pgrp, 0);
}

static inline int has_stopped_jobs(int pgrp)
{
	int retval = 0;
	struct task_struct * p;

	read_lock(&tasklist_lock);
	for_each_task(p) {
		if (p->pgrp != pgrp)
			continue;
		if (p->state != TASK_STOPPED)
			continue;
		retval = 1;
		break;
	}
	read_unlock(&tasklist_lock);
	return retval;
}

static inline void forget_original_parent(struct task_struct * father)
{
	struct task_struct * p;

	read_lock(&tasklist_lock);
	for_each_task(p) {
		if (p->p_opptr == father) {
			p->exit_signal = SIGCHLD;
			p->p_opptr = task[smp_num_cpus] ? : task[0]; /* init */
			if (p->pdeath_signal) send_sig(p->pdeath_signal, p, 0);
		}
	}
	read_unlock(&tasklist_lock);
}

static inline void close_files(struct files_struct * files)
{
	int i, j;

	j = 0;
	for (;;) {
		unsigned long set = files->open_fds.fds_bits[j];
		i = j * __NFDBITS;
		j++;
		if (i >= NR_OPEN)
			break;
		while (set) {
			if (set & 1) {
				struct file * file = files->fd[i];
				if (file) {
					files->fd[i] = NULL;
					close_fp(file);
				}
			}
			i++;
			set >>= 1;
		}
	}
}

extern kmem_cache_t *files_cachep;  

static inline void __exit_files(struct task_struct *tsk)
{
	struct files_struct * files = tsk->files;

	if (files) {
		tsk->files = NULL;
		if (!--files->count) {
			close_files(files);
			kmem_cache_free(files_cachep, files);
		}
	}
}

void exit_files(struct task_struct *tsk)
{
  __exit_files(tsk);
}

static inline void __exit_fs(struct task_struct *tsk)
{
	struct fs_struct * fs = tsk->fs;

	if (fs) {
		tsk->fs = NULL;
		if (!--fs->count) {
			dput(fs->root);
			dput(fs->pwd);
			kfree(fs);
		}
	}
}

void exit_fs(struct task_struct *tsk)
{
  __exit_fs(tsk);
}

static inline void __exit_sighand(struct task_struct *tsk)
{
	struct signal_struct * sig = tsk->sig;

	if (sig) {
		tsk->sig = NULL;
		if (atomic_dec_and_test(&sig->count))
			kfree(sig);
	}
}

void exit_sighand(struct task_struct *tsk)
{
	__exit_sighand(tsk);
}

static inline void __exit_mm(struct task_struct * tsk)
{
	struct mm_struct * mm = tsk->mm;

	/* Set us up to use the kernel mm state */
	if (mm != &init_mm) {
		flush_cache_mm(mm);
		flush_tlb_mm(mm);
		destroy_context(mm);
		tsk->mm = &init_mm;
		tsk->swappable = 0;
		SET_PAGE_DIR(tsk, swapper_pg_dir);
		mmput(mm);
	}
}

void exit_mm(struct task_struct *tsk)
{
	__exit_mm(tsk);
}

/*
 * Send signals to all our closest relatives so that they know
 * to properly mourn us..
 */
static void exit_notify(void)
{
	struct task_struct * p;

	forget_original_parent(current);
	/*
	 * Check to see if any process groups have become orphaned
	 * as a result of our exiting, and if they have any stopped
	 * jobs, send them a SIGHUP and then a SIGCONT.  (POSIX 3.2.2.2)
	 *
	 * Case i: Our father is in a different pgrp than we are
	 * and we were the only connection outside, so our pgrp
	 * is about to become orphaned.
	 */
	if ((current->p_pptr->pgrp != current->pgrp) &&
	    (current->p_pptr->session == current->session) &&
	    will_become_orphaned_pgrp(current->pgrp, current) &&
	    has_stopped_jobs(current->pgrp)) {
		kill_pg(current->pgrp,SIGHUP,1);
		kill_pg(current->pgrp,SIGCONT,1);
	}
	/* Let father know we died */
	notify_parent(current, current->exit_signal);

	/*
	 * This loop does two things:
	 *
  	 * A.  Make init inherit all the child processes
	 * B.  Check to see if any process groups have become orphaned
	 *	as a result of our exiting, and if they have any stopped
	 *	jobs, send them a SIGHUP and then a SIGCONT.  (POSIX 3.2.2.2)
	 */
	while ((p = current->p_cptr) != NULL) {
		current->p_cptr = p->p_osptr;
		p->p_ysptr = NULL;
		p->flags &= ~(PF_PTRACED|PF_TRACESYS);

		p->p_pptr = p->p_opptr;
		p->p_osptr = p->p_pptr->p_cptr;
		if (p->p_osptr)
			p->p_osptr->p_ysptr = p;
		p->p_pptr->p_cptr = p;
		if (p->state == TASK_ZOMBIE)
			notify_parent(p, p->exit_signal);
		/*
		 * process group orphan check
		 * Case ii: Our child is in a different pgrp
		 * than we are, and it was the only connection
		 * outside, so the child pgrp is now orphaned.
		 */
		if ((p->pgrp != current->pgrp) &&
		    (p->session == current->session) &&
		    is_orphaned_pgrp(p->pgrp) &&
		    has_stopped_jobs(p->pgrp)) {
			kill_pg(p->pgrp,SIGHUP,1);
			kill_pg(p->pgrp,SIGCONT,1);
		}
	}
	if (current->leader)
		disassociate_ctty(1);
}

NORET_TYPE void do_exit(long code)
{
	if (in_interrupt())
		printk("Aiee, killing interrupt handler\n");
fake_volatile:
	acct_process(code);
	current->flags |= PF_EXITING;
	del_timer(&current->real_timer);
	sem_exit();
	kerneld_exit();
	__exit_mm(current);
#if CONFIG_AP1000
	exit_msc(current);
#endif
	__exit_files(current);
	__exit_fs(current);
	__exit_sighand(current);
	exit_thread();
	current->state = TASK_ZOMBIE;
	current->exit_code = code;
	exit_notify();
#ifdef DEBUG_PROC_TREE
	audit_ptree();
#endif
	if (current->exec_domain && current->exec_domain->module)
		__MOD_DEC_USE_COUNT(current->exec_domain->module);
	if (current->binfmt && current->binfmt->module)
		__MOD_DEC_USE_COUNT(current->binfmt->module);
	schedule();
/*
 * In order to get rid of the "volatile function does return" message
 * I did this little loop that confuses gcc to think do_exit really
 * is volatile. In fact it's schedule() that is volatile in some
 * circumstances: when current->state = ZOMBIE, schedule() never
 * returns.
 *
 * In fact the natural way to do all this is to have the label and the
 * goto right after each other, but I put the fake_volatile label at
 * the start of the function just in case something /really/ bad
 * happens, and the schedule returns. This way we can try again. I'm
 * not paranoid: it's just that everybody is out to get me.
 */
	goto fake_volatile;
}

asmlinkage int sys_exit(int error_code)
{
	lock_kernel();
	do_exit((error_code&0xff)<<8);
	unlock_kernel();
}

asmlinkage int sys_wait4(pid_t pid,unsigned int * stat_addr, int options, struct rusage * ru)
{
	int flag, retval;
	struct wait_queue wait = { current, NULL };
	struct task_struct *p;

	if (stat_addr) {
		if(verify_area(VERIFY_WRITE, stat_addr, sizeof(*stat_addr)))
			return -EFAULT;
	}
	if (ru) {
		if(verify_area(VERIFY_WRITE, ru, sizeof(*ru)))
			return -EFAULT;
	}

	if (options & ~(WNOHANG|WUNTRACED|__WCLONE))
		return -EINVAL;

	add_wait_queue(&current->wait_chldexit,&wait);
repeat:
	flag = 0;
	read_lock(&tasklist_lock);
 	for (p = current->p_cptr ; p ; p = p->p_osptr) {
		if (pid>0) {
			if (p->pid != pid)
				continue;
		} else if (!pid) {
			if (p->pgrp != current->pgrp)
				continue;
		} else if (pid != -1) {
			if (p->pgrp != -pid)
				continue;
		}
		/* wait for cloned processes iff the __WCLONE flag is set */
		if ((p->exit_signal != SIGCHLD) ^ ((options & __WCLONE) != 0))
			continue;
		flag = 1;
		switch (p->state) {
			case TASK_STOPPED:
				if (!p->exit_code)
					continue;
				if (!(options & WUNTRACED) && !(p->flags & PF_PTRACED))
					continue;
				read_unlock(&tasklist_lock);
				if (ru != NULL)
					getrusage(p, RUSAGE_BOTH, ru);
				if (stat_addr)
					__put_user((p->exit_code << 8) | 0x7f,
						   stat_addr);
				p->exit_code = 0;
				retval = p->pid;
				goto end_wait4;
			case TASK_ZOMBIE:
				current->times.tms_cutime += p->times.tms_utime + p->times.tms_cutime;
				current->times.tms_cstime += p->times.tms_stime + p->times.tms_cstime;
				read_unlock(&tasklist_lock);
				if (ru != NULL)
					getrusage(p, RUSAGE_BOTH, ru);
				if (stat_addr)
					__put_user(p->exit_code, stat_addr);
				retval = p->pid;
				if (p->p_opptr != p->p_pptr) {
					/* Note this grabs tasklist_lock
					 * as a writer... (twice!)
					 */
					REMOVE_LINKS(p);
					p->p_pptr = p->p_opptr;
					SET_LINKS(p);
					notify_parent(p, SIGCHLD);
				} else
					release(p);
#ifdef DEBUG_PROC_TREE
				audit_ptree();
#endif
				goto end_wait4;
			default:
				continue;
		}
	}
	read_unlock(&tasklist_lock);
	if (flag) {
		retval = 0;
		if (options & WNOHANG)
			goto end_wait4;
		retval = -ERESTARTSYS;
		if (signal_pending(current))
			goto end_wait4;
		current->state=TASK_INTERRUPTIBLE;
		schedule();
		goto repeat;
	}
	retval = -ECHILD;
end_wait4:
	remove_wait_queue(&current->wait_chldexit,&wait);
	return retval;
}

#ifndef __alpha__

/*
 * sys_waitpid() remains for compatibility. waitpid() should be
 * implemented by calling sys_wait4() from libc.a.
 */
asmlinkage int sys_waitpid(pid_t pid,unsigned int * stat_addr, int options)
{
	return sys_wait4(pid, stat_addr, options, NULL);
}

#endif
