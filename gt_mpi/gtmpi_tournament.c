#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/*
    From the MCS Paper: A scalable, distributed tournament barrier with only local spinning

    type round_t = record
        role : (winner, loser, bye, champion, dropout)
	opponent : ^Boolean
	flag : Boolean
    shared rounds : array [0..P-1][0..LogP] of round_t
        // row vpid of rounds is allocated in shared memory
	// locally accessible to processor vpid

    processor private sense : Boolean := true
    processor private vpid : integer // a unique virtual processor index

    //initially
    //    rounds[i][k].flag = false for all i,k
    //rounds[i][k].role =
    //    winner if k > 0, i mod 2^k = 0, i + 2^(k-1) < P , and 2^k < P
    //    bye if k > 0, i mode 2^k = 0, and i + 2^(k-1) >= P
    //    loser if k > 0 and i mode 2^k = 2^(k-1)
    //    champion if k > 0, i = 0, and 2^k >= P
    //    dropout if k = 0
    //    unused otherwise; value immaterial
    //rounds[i][k].opponent points to
    //    round[i-2^(k-1)][k].flag if rounds[i][k].role = loser
    //    round[i+2^(k-1)][k].flag if rounds[i][k].role = winner or champion
    //    unused otherwise; value immaterial
    procedure tournament_barrier
        round : integer := 1
	loop   //arrival
	    case rounds[vpid][round].role of
	        loser:
	            rounds[vpid][round].opponent^ :=  sense
		    repeat until rounds[vpid][round].flag = sense
		    exit loop
   	        winner:
	            repeat until rounds[vpid][round].flag = sense
		bye:  //do nothing
		champion:
	            repeat until rounds[vpid][round].flag = sense
		    rounds[vpid][round].opponent^ := sense
		    exit loop
		dropout: // impossible
	    round := round + 1
	loop  // wakeup
	    round := round - 1
	    case rounds[vpid][round].role of
	        loser: // impossible
		winner:
		    rounds[vpid[round].opponent^ := sense
		bye: // do nothing
		champion: // impossible
		dropout:
		    exit loop
	sense := not sense
*/

int r;

MPI_Status * Stat;

int power (int base, int exp) {
	int res = 1;

	if (exp == 0) {
		return res;
	}

	while (exp != 0) {
		res *= base;
		exp--;
	}
	return res;
}

int ceil_log2(unsigned long long x)
{
  static const unsigned long long t[6] = {
    0xFFFFFFFF00000000ull,
    0x00000000FFFF0000ull,
    0x000000000000FF00ull,
    0x00000000000000F0ull,
    0x000000000000000Cull,
    0x0000000000000002ull
  };

  int y = (((x & (x - 1)) == 0) ? 0 : 1);
  int j = 32;
  int i;

  for (i = 0; i < 6; i++) {
    int k = (((x & t[i]) == 0) ? 0 : j);
    y += k;
    x >>= k;
    j >>= 1;
  }

  return y;
}

void gtmpi_init(int num_threads){

	Stat = (MPI_Status*) malloc(num_threads * sizeof(MPI_Status));

	r = ceil_log2(num_threads);

}

void gtmpi_barrier(){

	int np, k, id, temp, z, tag=1;

	MPI_Comm_size(MPI_COMM_WORLD, &np);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	for (k = 0; k < r; k++){
			if ( (id % power(2, (k+1))) == power(2, k) ) {
				//Rounds[x][k].role = loser;
				temp = id - power(2, k);
				MPI_Send(NULL, 0, MPI_INT, temp, tag, MPI_COMM_WORLD);
				MPI_Recv(NULL, 0, MPI_INT, temp, tag, MPI_COMM_WORLD, &Stat[temp]);
			}
			if ( ((id % power(2, (k+1))) == 0) && ((id % power(2, k)) < np) && (power(2, (k+1)) < np) && (id + power(2, k) < np) ) {
				//Rounds[x][k].role = winner;
				temp = id + power(2, k);
				MPI_Recv(NULL, 0, MPI_INT, temp, tag, MPI_COMM_WORLD, &Stat[temp]);
			}
			if ( id == 0 && power(2, (k+1)) >= np ) {
				//Rounds[x][k].role = champion;
				temp = id + power(2, k);
				MPI_Recv(NULL, 0, MPI_INT, temp, tag, MPI_COMM_WORLD, &Stat[temp]);
				MPI_Send(NULL, 0, MPI_INT, temp, tag, MPI_COMM_WORLD);
			}
	}

	for (k = r; k >= 0; k--){
			if ( (id % power(2, (k+1))) == 0 && (id % power(2, k)) < np && power(2, (k+1)) < np && (id + power(2, k) < np) ) {
				//Rounds[x][k].role = winner;
				temp = id + power(2, k);
				MPI_Send(NULL, 0, MPI_INT, temp, tag, MPI_COMM_WORLD);
			}
	}

}

void gtmpi_finalize(){

	free(Stat);

}

