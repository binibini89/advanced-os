#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

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

/*
    Here are the results from my tests:
    
    barriers = 1000000, threads = 7

    optimized:
    1) 19s
    2) 18s
    3) 18s
    4) 18s
    5) 19s
    6) 18s
    Avg. 18.33s

    unoptimized:
    1) 18s
    2) 19s
    3) 19s
    4) 19s
    5) 18s
    6) 18s
    Avg. 18.5s
*/

static MPI_Status* status_array;
static int P;

void gtmpi_init(int num_threads){
  P = num_threads;
  status_array = (MPI_Status*) malloc(P * sizeof(MPI_Status));
}

void gtmpi_barrier(){
  int vpid, i;

  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);
    
    /*
        The optimization I made was focused on reducing the number of instructions needed to send and 
        receive messages from all the threads. Before there were two calls to MPI_Send and MPI_Recv and 
        after there is only one. The results from my tests show that the optimization I made is slightly 
        faster.
    */

  for (i = 0; i < P; i++) {
	   if (i == vpid) {
		   continue;
       }
	   MPI_Send(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD);
  }

  for (i = 0; i < P; i++) {
	   if (i == vpid) {
	       continue;
	   }
	   MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status_array[i]);
  }

/*
  for(i = 0; i < vpid; i++)
    MPI_Send(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD);
  for(i = vpid + 1; i < P; i++)
    MPI_Send(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD);

  for(i = 0; i < vpid; i++)
    MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status_array[i]);
  for(i = vpid + 1; i < P; i++)
    MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status_array[i-1]);
*/
}

void gtmpi_finalize(){
  if(status_array != NULL){
    free(status_array);
  }
}