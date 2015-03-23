/*
 * gtthread.h
 *
 *  Created on: Sep 16, 2014
 *      Author: michael
 */

#ifndef GTTHREAD_H
#define GTTHREAD_H

#include "steque.h"
#include <ucontext.h>

/* Define gtthread_t and gtthread_mutex_t types here */

typedef unsigned long counter_t;

struct gtthread {
	counter_t gt_id;
	void * exit_value;
	int done;
	int cancel_pend;
	int on_mu_que;
};

typedef struct gtthread * gtthread_t;

struct gtthread_mutex {
	counter_t gtmutex_id;
	int mu_lock;
};

typedef struct gtthread_mutex gtthread_mutex_t;

steque_t queue;

static sigset_t alrm;

void spincpu(double secs);
void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);
int  gtthread_join(gtthread_t thread, void **status);
void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);


int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);
int  gtthread_mutex_destroy(gtthread_mutex_t *mutex);
#endif
