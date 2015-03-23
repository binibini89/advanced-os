#include <omp.h>
#include "gtmp.h"
#include <stdlib.h>

/*
    From the MCS Paper: A sense-reversing centralized barrier

    shared count : integer := P
    shared sense : Boolean := true
    processor private local_sense : Boolean := true

    procedure central_barrier
        local_sense := not local_sense // each processor toggles its own sense
	if fetch_and_decrement (&count) = 1
	    count := P
	    sense := local_sense // last processor toggles global sense
        else
           repeat until sense = local_sense
*/
typedef enum { false, true } bool;
static bool global_sense = true;
bool * local_sense;
int count;
int * pCount = &count;


void gtmp_init(int num_threads){

	local_sense = calloc(num_threads, sizeof(bool));

	int i;

	for (i = 0; i < num_threads; i++) {
	local_sense[i] = true;
	}

	count = num_threads;

}

void gtmp_barrier(){

	local_sense[omp_get_thread_num()] = !local_sense[omp_get_thread_num()];

	__sync_fetch_and_sub(pCount, 1);

	if (count == 0) {
		count = omp_get_num_threads();
		global_sense = local_sense[omp_get_thread_num()];
	}
	else {
		while (global_sense != local_sense[omp_get_thread_num()]) {}
	}

}

void gtmp_finalize(){

	free(local_sense);

}
