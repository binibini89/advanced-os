#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

/*

    From the MCS Paper: A software combining tree barrier with optimized wakeup

    type node = record
        k : integer //fan in of this node
	count : integer // initialized to k
	locksense : Boolean // initially false
	parent : ^node // pointer to parent node; nil if root

	shared nodes : array [0..P-1] of node
	    //each element of nodes allocated in a different memory module or cache line

	processor private sense : Boolean := true
	processor private mynode : ^node // my group's leaf in the combining tree

	procedure combining_barrier
	    combining_barrier_aux (mynode) // join the barrier
	    sense := not sense             // for next barrier


	procedure combining_barrier_aux (nodepointer : ^node)
	    with nodepointer^ do
	        if fetch_and_decrement (&count) = 1 // last one to reach this node
		    if parent != nil
		        combining_barrier_aux (parent)
		    count := k // prepare for next barrier
		    locksense := not locksense // release waiting processors
		repeat until locksense = sense
*/

/*
 * I made two major changes, and one minor tweak to a change, to the supplied code. The first
 * (optimization 1) was taking out the OMP Critical section and replacing it with a fetch and
 * decrement instruction. I made this change because that OMP directive will block all other threads
 * entering into that critical section. The fetch and decrement operation is atomic and therefore nothing
 * will interrupt it while it executes. The second change (optimization 2) was making the sense variable an
 * array for a thread specific sense variable. The idea behind this was to decrease contention of the
 * sense variable. The minor change I made to the second optimization (optimization 2.1) was instead of
 * using malloc() to allocate space for the sense variables I used posix_memalign. This was in effort to
 * place spin variables on seperate cache lines. All three of these
 * optimizations were implemented independently. I ran the barriers 10000 times with 8 threads. I used
 * gettimeofday() to record the begining and end times before and after all the barriers. I did each
 * optimization 5 times and averaged the times recorded. The results are below. Unchanged had the greatest
 * difference in times it took to complete. When only optimization 1 was implemented i got the best times.
 * Using posix_memalign (optimization 2.1) recorded the worst times for me. I believe that I may not have
 * been using the posix_memalign function as intended and therefore left it out when combining the two
 * optimiztions made (optimization 1+2).
 *
unchanged:
1) 190s
2) 172s
3) 167s
4) 184s
5) 172s
Avg) 177s

optimization 1:
1) 177s
2) 179s
3) 177s
4) 175s
5) 172s
Avg) 176s

optimization 2:
1) 176s
2) 171s
3) 184s
4) 186s
5) 178s
Avg) 179s

optimization 2.1:
1) 191s
2) 187s
3) 180s
4) 182s
5) 186s
Avg) 185.2s

optimization 1+2:
1) 178s
2) 182s
3) 188s
4) 182s
5) 179s
Avg) 181.8s
*/

typedef struct _node_t{
  int k;
  int count;
  int locksense;
  struct _node_t* parent;
} node_t;

static int num_leaves;
static node_t* nodes;

  /*
   * Optimization 2
   */
int * sense;

void gtmp_barrier_aux(node_t* node);

node_t* _gtmp_get_node(int i){
  return &nodes[i];
}

void gtmp_init(int num_threads){
  int i, v, x, num_nodes;
  node_t* curnode;

  /*Setting constants */
  v = 1;
  while( v < num_threads)
    v *= 2;

  num_nodes = v - 1;
  num_leaves = v/2;

  /* Setting up the tree */
  nodes = (node_t*) malloc(num_nodes * sizeof(node_t));

  sense = (int*) malloc(num_threads * sizeof(int));

  /*
   * Optimization 2.1
   */
  //posix_memalign((void **)&sense, LEVEL1_DCACHE_LINESIZE, num_threads * sizeof(int));

  for(i = 0; i < num_nodes; i++){
    curnode = _gtmp_get_node(i);
    curnode->k = i < num_threads - 1 ? 2 : 1;
    curnode->count = curnode->k;
    curnode->locksense = 0;
    curnode->parent = _gtmp_get_node((i-1)/2);
  }

  for(x = 0; x < num_threads; x++){
	  sense[x] = 1;
  }

  curnode = _gtmp_get_node(0);
  curnode->parent = NULL;
}

void gtmp_barrier(){
  node_t* mynode;
  //int sense;

  mynode = _gtmp_get_node(num_leaves - 1 + (omp_get_thread_num() % num_leaves));

  /*
     Rather than correct the sense variable after the call to
     the auxilliary method, we set it correctly before.
   */
  //sense = !mynode->locksense;

  gtmp_barrier_aux(mynode);

  sense[omp_get_thread_num()] = !sense[omp_get_thread_num()];
}

void gtmp_barrier_aux(node_t* node){

  /*
   * Optimization 1
   */
  int * test = &node->count;
  //int test;

/*
#pragma omp critical
{
  test = node->count;
  node->count--;
}
*/

  //if( 1 == test ){
  if( __sync_fetch_and_sub(test, 1) == 1 ){
    if(node->parent != NULL)
      gtmp_barrier_aux(node->parent);
    node->count = node->k;
    node->locksense = !node->locksense;
  }
  while (node->locksense != sense[omp_get_thread_num()]);
}

void gtmp_finalize(){
  free(nodes);
  free(sense);
}

