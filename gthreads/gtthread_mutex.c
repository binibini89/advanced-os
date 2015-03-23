/*
 * gtthread_mutex.c
 *
 *  Created on: Sep 16, 2014
 *      Author: michael
 */


/**********************************************************************
gtthread_mutex.c.

This file contains the implementation of the mutex subset of the
gtthreads library.  The locks can be implemented with a simple queue.
 **********************************************************************/

/*
  Include as needed
*/


#include "gtthread.h"
#include <stdlib.h>
#include <stdio.h>



steque_t mutex_ar[2] = {
							{},
							{}
						};


counter_t mutex_temp_id = 0;




/*
  The gtthread_mutex_init() function is analogous to
  pthread_mutex_init with the default parameters enforced.
  There is no need to create a static initializer analogous to
  PTHREAD_MUTEX_INITIALIZER.
 */
int gtthread_mutex_init(gtthread_mutex_t* mutex){

	struct gtthread_mutex * new_mu;

	new_mu = (struct gtthread_mutex *) malloc(sizeof(struct gtthread_mutex));

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	mutex_temp_id++;

	new_mu->gtmutex_id = mutex_temp_id;

	steque_init(&mutex_ar[new_mu->gtmutex_id]);

	new_mu->mu_lock = 0;

	mutex = new_mu;

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	return 0;

}

/*
  The gtthread_mutex_lock() is analogous to pthread_mutex_lock.
  Returns zero on success.
 */
int gtthread_mutex_lock(gtthread_mutex_t* mutex){

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	if (mutex->mu_lock == 0) {
		mutex->mu_lock = 1;
		return 0;
	}

	struct gtthread *cur_t;

	cur_t = steque_front(&queue);

	steque_enqueue(&mutex_ar[mutex->gtmutex_id], cur_t);

	cur_t->on_mu_que = 1;

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	while (cur_t->on_mu_que == 1) {


	}

	mutex->mu_lock = 1;


	return 0;

}

/*
  The gtthread_mutex_unlock() is analogous to pthread_mutex_unlock.
  Returns zero on success.
 */
int gtthread_mutex_unlock(gtthread_mutex_t *mutex){

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	mutex->mu_lock = 0;

	if (steque_size(&mutex_ar[mutex->gtmutex_id]) > 0) {
		struct gtthread *cur_t;

		cur_t = steque_pop(&mutex_ar[mutex->gtmutex_id]);

		cur_t->on_mu_que = 0;
	}

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	return 0;

}

/*
  The gtthread_mutex_destroy() function is analogous to
  pthread_mutex_destroy and frees any resourcs associated with the mutex.
*/
int gtthread_mutex_destroy(gtthread_mutex_t *mutex){

	sigprocmask(SIG_BLOCK, &alrm, NULL);


	mutex->gtmutex_id = -1;

	steque_destroy(&mutex_ar[mutex->gtmutex_id]);

	free (mutex);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);


	return 0;

}
