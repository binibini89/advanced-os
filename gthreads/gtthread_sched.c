/*
 * gtthread_sched.c
 *
 *  Created on: Sep 16, 2014
 *      Author: michael
 */


/**********************************************************************
gtthread_sched.c.

This file contains the implementation of the scheduling subset of the
gtthreads library.  A simple round-robin queue should be used.
 **********************************************************************/
/*
  Include as needed
*/

#include "gtthread.h"
#include "steque.h"
#include <ucontext.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>




/*
   Students should define global variables and helper functions as
   they see fit.
 */

counter_t temp_id;

steque_t waiting_queue;


void gtthread_fun(void *(*start_routine)(void *),void *arg){

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	struct gtthread *thrd;

	thrd = steque_front(&queue);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);


	if (thrd->cancel_pend == 1){
		gtthread_exit(NULL);
	}


	void * retval = (*start_routine)(arg);

	thrd->done = 1;


	printf("thread %lu done is %d!\n", thrd->gt_id, thrd->done);
			  fflush(stdout);

	struct gtthread * jn_t;

	sigprocmask(SIG_BLOCK, &alrm, NULL);


	if (steque_size(&queue) == 1) {

		if (steque_size(&waiting_queue) == 1){

			jn_t = steque_pop(&waiting_queue);

			steque_enqueue(&queue, jn_t);

			sigprocmask(SIG_UNBLOCK, &alrm, NULL);

			printf("super duper!\n");
								  fflush(stdout);


			gtthread_exit(retval);
		}

		sigprocmask(SIG_BLOCK, &alrm, NULL);

		if (steque_size(&waiting_queue) == 2){

			steque_cycle(&waiting_queue);

			jn_t = steque_pop(&waiting_queue);

			steque_enqueue(&queue, jn_t);

			sigprocmask(SIG_UNBLOCK, &alrm, NULL);

			gtthread_exit(retval);
		}

		sigprocmask(SIG_BLOCK, &alrm, NULL);

		if (steque_size(&waiting_queue) == 3){

			steque_cycle(&waiting_queue);
			steque_cycle(&waiting_queue);

			jn_t = steque_pop(&waiting_queue);

			steque_enqueue(&queue, jn_t);

			sigprocmask(SIG_UNBLOCK, &alrm, NULL);

			gtthread_exit(retval);
		}

		sigprocmask(SIG_BLOCK, &alrm, NULL);

				if (steque_size(&waiting_queue) == 4){

					steque_cycle(&waiting_queue);
					steque_cycle(&waiting_queue);
					steque_cycle(&waiting_queue);

					jn_t = steque_pop(&waiting_queue);

					steque_enqueue(&queue, jn_t);

					sigprocmask(SIG_UNBLOCK, &alrm, NULL);

					gtthread_exit(retval);
				}


	}



	thrd->exit_value = retval = 0;


	if (thrd->gt_id == 0){
			exit(0);
	}
	else {
		gtthread_exit(retval);
	}

}


ucontext_t uctx_ar[4] = {
							{},
							{},
							{},
							{}
						};


void alram_handler (int signum) {
	sigprocmask(SIG_BLOCK, &alrm, NULL);
	gtthread_yield();
	sigprocmask(SIG_UNBLOCK, &alrm, NULL);
}




/*
  The gtthread_init() function does not have a corresponding pthread equivalent.
  It must be called from the main thread before any other GTThreads
  functions are called. It allows the caller to specify the scheduling
  period (quantum in micro second), and may also perform any other
  necessary initialization.  If period is zero, then thread switching should
  occur only on calls to gtthread_yield().

  Recall that the initial thread of the program (i.e. the one running
  main() ) is a thread like any other. It should have a
  gtthread_t that clients can retrieve by calling gtthread_self()
  from the initial thread, and they should be able to specify it as an
  argument to other GTThreads functions. The only difference in the
  initial thread is how it behaves when it executes a return
  instruction. You can find details on this difference in the man page
  for pthread_create.
 */
void gtthread_init(long period){

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	steque_init(&queue);
	steque_init(&waiting_queue);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	gtthread_t thd1;

	thd1 = (struct gtthread *) malloc(sizeof(struct gtthread));

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	temp_id = 0;

	thd1->gt_id = temp_id;

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);



	getcontext(&uctx_ar[thd1->gt_id]);

	uctx_ar[thd1->gt_id].uc_stack.ss_sp = (char*) malloc(SIGSTKSZ);
	uctx_ar[thd1->gt_id].uc_stack.ss_size = SIGSTKSZ;


	uctx_ar[thd1->gt_id].uc_link = NULL;

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	steque_enqueue(&queue, thd1);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);


	struct itimerval * timer;

	struct sigaction action;

	sigemptyset(&alrm);
	sigaddset(&alrm, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	memset (&action, 0, sizeof(action));
	action.sa_handler = &alram_handler;
	sigaction(SIGALRM, &action, NULL);

	timer = (struct itimerval*) malloc(sizeof(struct itimerval));
	timer->it_value.tv_usec = timer->it_interval.tv_usec = period;

	setitimer(ITIMER_REAL, timer, NULL);


}


/*
  The gtthread_create() function mirrors the pthread_create() function,
  only default attributes are always assumed.
 */
int gtthread_create(gtthread_t *thread,
		    void *(*start_routine)(void *),
		    void *arg){

	struct gtthread *new_th, *active_th;

	new_th = (struct gtthread *) malloc(sizeof(struct gtthread));

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	temp_id++;

	new_th->gt_id = temp_id;

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	getcontext(&uctx_ar[new_th->gt_id]);

	sigprocmask(SIG_BLOCK, &alrm, NULL);


	active_th = steque_front(&queue);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);



	uctx_ar[new_th->gt_id].uc_stack.ss_sp = (char*) malloc(SIGSTKSZ);
	uctx_ar[new_th->gt_id].uc_stack.ss_size = SIGSTKSZ;


	uctx_ar[new_th->gt_id].uc_link = &uctx_ar[active_th->gt_id];



	makecontext(&uctx_ar[new_th->gt_id], (void (*) (void)) gtthread_fun, 2, (*start_routine), arg);


	sigprocmask(SIG_BLOCK, &alrm, NULL);


	steque_enqueue(&queue, new_th);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	*thread = new_th;

	return 0;


}

/*
  The gtthread_join() function is analogous to pthread_join.
  All gtthreads are joinable.
 */
int gtthread_join(gtthread_t thread, void **status){

	if (thread->done == 1) {
	        return 0;
	    }

	struct gtthread *cur_t, *jn_t, *next_t;

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	if (steque_size(&queue) < 1) {
		return 0;
	}

	cur_t = steque_front(&queue);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	if (gtthread_equal(cur_t, thread) == 0) {
		return 0;
	}


	sigprocmask(SIG_BLOCK, &alrm, NULL);

	if (thread->done != 1) {

		jn_t = steque_pop(&queue);

		steque_enqueue(&waiting_queue, jn_t);

		next_t = steque_front(&queue);

		sigprocmask(SIG_UNBLOCK, &alrm, NULL);

		printf("great!\n");
		  fflush(stdout);

		swapcontext(&uctx_ar[jn_t->gt_id], &uctx_ar[next_t->gt_id]);


		printf("excellent!\n");
		  fflush(stdout);

	}


	printf("sweet!\n");
			  fflush(stdout);

	status = thread->exit_value;


	return 0;

}

/*
  The gtthread_exit() function is analogous to pthread_exit.
 */
void gtthread_exit(void* retval){

	struct gtthread * cur_t;

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	cur_t = steque_pop(&queue);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);




	cur_t->exit_value = retval;




	sigprocmask(SIG_BLOCK, &alrm, NULL);


	struct gtthread *next_t = steque_front(&queue);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	swapcontext(&uctx_ar[cur_t->gt_id], &uctx_ar[next_t->gt_id]);

	free (uctx_ar[cur_t->gt_id].uc_stack.ss_sp);

	free (cur_t);

	cur_t->gt_id = -1;





}


/*
  The gtthread_yield() function is analogous to pthread_yield, causing
  the calling thread to relinquish the cpu and place itself at the
  back of the schedule queue.
 */
void gtthread_yield(void){

	struct gtthread *cur_t;

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	cur_t = steque_front(&queue);

	steque_cycle(&queue);


	struct gtthread *next_t = steque_front(&queue);


	sigprocmask(SIG_UNBLOCK, &alrm, NULL);

	swapcontext(&uctx_ar[cur_t->gt_id], &uctx_ar[next_t->gt_id]);


}

/*
  The gtthread_yield() function is analogous to pthread_equal,
  returning zero if the threads are the same and non-zero otherwise.
 */
int  gtthread_equal(gtthread_t t1, gtthread_t t2){

		if (t1->gt_id == t2->gt_id) {
			return 0;
		}
		else {
			return -1;
		}

}

/*
  The gtthread_cancel() function is analogous to pthread_cancel,
  allowing one thread to terminate another asynchronously.
 */
int  gtthread_cancel(gtthread_t thread){

	thread->cancel_pend = 1;

	return 0;

}

/*
  Returns calling thread.
 */
gtthread_t gtthread_self(void){

	struct gtthread *cur_t;

	sigprocmask(SIG_BLOCK, &alrm, NULL);

	cur_t = steque_front(&queue);

	printf("im thread: %lu\n", cur_t->gt_id);
	  fflush(stdout);

	  printf("waiting queue size: %d and ready queue size: %d\n",steque_size(&waiting_queue), steque_size(&queue));
	    fflush(stdout);

	sigprocmask(SIG_UNBLOCK, &alrm, NULL);


	return cur_t;

}
