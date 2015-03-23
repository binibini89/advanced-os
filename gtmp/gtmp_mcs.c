#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

/*
    From the MCS Paper: A scalable, distributed tree-based barrier with only local spinning.

    type treenode = record
        parentsense : Boolean
	parentpointer : ^Boolean
	childpointers : array [0..1] of ^Boolean
	havechild : array [0..3] of Boolean
	childnotready : array [0..3] of Boolean
	dummy : Boolean //pseudo-data

    shared nodes : array [0..P-1] of treenode
        // nodes[vpid] is allocated in shared memory
        // locally accessible to processor vpid
    processor private vpid : integer // a unique virtual processor index
    processor private sense : Boolean

    // on processor i, sense is initially true
    // in nodes[i]:
    //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
    //    parentpointer = &nodes[floor((i-1)/4].childnotready[(i-1) mod 4],
    //        or dummy if i = 0
    //    childpointers[0] = &nodes[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &nodes[2*i+2].parentsense, or &dummy if 2*i+2 >= P
    //    initially childnotready = havechild and parentsense = false

    procedure tree_barrier
        with nodes[vpid] do
	    repeat until childnotready = {false, false, false, false}
	    childnotready := havechild //prepare for next barrier
	    parentpointer^ := false //let parent know I'm ready
	    // if not root, wait until my parent signals wakeup
	    if vpid != 0
	        repeat until parentsense = sense
	    // signal children in wakeup tree
	    childpointers[0]^ := sense
	    childpointers[1]^ := sense
	    sense := not sense
*/

typedef enum { false, true } bool;

struct Treenode {
	bool parent_sense;
	bool * parent_pointer;
	bool * child_pointers[2];
	bool have_child[4];
	bool child_not_ready[4];
	bool dummy;
};

struct Treenode * nodes;

bool * sense;


void gtmp_init(int num_threads){

	nodes = calloc(num_threads, sizeof(struct Treenode));

	sense = calloc(num_threads, sizeof(bool));

	int x;

	for (x = 0; x < num_threads; x++) {
		sense[x] = true;
	}

	int i, j;

	for (i = 0; i < num_threads; i++) {
		for (j = 0; j < 4; j++) {
			nodes[i].have_child[j] = (4 * i + j + 1 < num_threads) ? true : false;
			nodes[i].child_not_ready[j] = nodes[i].have_child[j];
		}
		nodes[i].parent_pointer = (i != 0) ? &nodes[(int)((i-1)/4)].child_not_ready[(i-1) % 4] : &nodes[i].dummy;
		nodes[i].child_pointers[0] = (2*i+1 < num_threads) ? &nodes[2*i+1].parent_sense : &nodes[i].dummy;
		nodes[i].child_pointers[1] = (2*i+2 < num_threads) ? &nodes[2*i+2].parent_sense : &nodes[i].dummy;
		nodes[i].parent_sense = false;
	}

}

void gtmp_barrier(){

	int i;

	for (i = 0; i < 4; i++) {
		while (nodes[omp_get_thread_num()].child_not_ready[i] != false) {

		}
	}

	int j;

	for (j = 0; j < 4; j++) {
		nodes[omp_get_thread_num()].child_not_ready[j] = nodes[omp_get_thread_num()].have_child[j];
	}

	*nodes[omp_get_thread_num()].parent_pointer = false;

	if (omp_get_thread_num() != 0) {
		while (nodes[omp_get_thread_num()].parent_sense != sense[omp_get_thread_num()]) {

		}
	}

	*nodes[omp_get_thread_num()].child_pointers[0] = sense[omp_get_thread_num()];
	*nodes[omp_get_thread_num()].child_pointers[1] = sense[omp_get_thread_num()];
	sense[omp_get_thread_num()] = !sense[omp_get_thread_num()];

}

void gtmp_finalize(){

	free(nodes);

	free(sense);

}