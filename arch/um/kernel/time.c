/* 
 * Copyright (C) 2000, 2001, 2002 Jeff Dike (jdike@karaya.com)
 * Licensed under the GPL
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include "user_util.h"
#include "kern_util.h"
#include "user.h"
#include "process.h"
#include "signal_user.h"
#include "time_user.h"

extern struct timeval xtime;

void timer(void)
{
	gettimeofday(&xtime, NULL);
}

static void set_interval(int timer_type)
{
	struct itimerval interval;

	interval.it_interval.tv_sec = 0;
	interval.it_interval.tv_usec = 1000000/hz();
	interval.it_value.tv_sec = 0;
	interval.it_value.tv_usec = 1000000/hz();
	if(setitimer(timer_type, &interval, NULL) == -1)
		panic("setitimer failed - errno = %d\n", errno);
}

void enable_timer(void)
{
	struct itimerval enable = ((struct itimerval) { { 0, 1000000/hz() },
							{ 0, 1000000/hz() }});
	if(setitimer(ITIMER_VIRTUAL, &enable, NULL))
		printk("enable_timer - setitimer failed, errno = %d\n",
		       errno);
}

void switch_timers(int to_real)
{
	struct itimerval disable = ((struct itimerval) { { 0, 0 }, { 0, 0 }});
	struct itimerval enable = ((struct itimerval) { { 0, 1000000/hz() },
							{ 0, 1000000/hz() }});
	int old, new;

	if(to_real){
		old = ITIMER_VIRTUAL;
		new = ITIMER_REAL;
	}
	else {
		old = ITIMER_REAL;
		new = ITIMER_VIRTUAL;
	}

	if((setitimer(old, &disable, NULL) < 0) ||
	   (setitimer(new, &enable, NULL)))
		printk("switch_timers - setitimer failed, errno = %d\n",
		       errno);
}

void idle_timer(void)
{
	if(signal(SIGVTALRM, SIG_IGN) == SIG_ERR)
		panic("Couldn't unset SIGVTALRM handler");
	
	set_handler(SIGALRM, (__sighandler_t) alarm_handler, 
		    SA_RESTART, SIGUSR1, SIGIO, SIGWINCH, SIGVTALRM, -1);
	set_interval(ITIMER_REAL);
}

void user_time_init(void)
{
	if(signal(SIGVTALRM, (__sighandler_t) alarm_handler) == SIG_ERR)
		panic("Couldn't set SIGVTALRM handler");
	set_interval(ITIMER_VIRTUAL);
}

void time_init(void)
{
	if(signal(SIGVTALRM, boot_timer_handler) == SIG_ERR)
		panic("Couldn't set SIGVTALRM handler");
	set_interval(ITIMER_VIRTUAL);
}

struct timeval local_offset = { 0, 0 };

void do_gettimeofday(struct timeval *tv)
{
	time_lock();
	gettimeofday(tv, NULL);
	timeradd(tv, &local_offset, tv);
	time_unlock();
}

void do_settimeofday(struct timeval *tv)
{
	struct timeval now;

	time_lock();
	gettimeofday(&now, NULL);
	timersub(tv, &now, &local_offset);
	time_unlock();
}

void idle_sleep(int secs)
{
	struct timespec ts;

	ts.tv_sec = secs;
	ts.tv_nsec = 0;
	nanosleep(&ts, &ts);
}

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-file-style: "linux"
 * End:
 */