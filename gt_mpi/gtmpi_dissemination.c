#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/*
    From the MCS Paper: The scalable, distributed dissemination barrier with only local spinning.

    type flags = record
        myflags : array [0..1] of array [0..LogP - 1] of Boolean
	partnerflags : array [0..1] of array [0..LogP - 1] of ^Boolean

    processor private parity : integer := 0
    processor private sense : Boolean := true
    processor private localflags : ^flags

    shared allnodes : array [0..P-1] of flags
        //allnodes[i] is allocated in shared memory
	//locally accessible to processor i

    //on processor i, localflags points to allnodes[i]
    //initially allnodes[i].myflags[r][k] is false for all i, r, k
    //if j = (i+2^k) mod P, then for r = 0 , 1:
    //    allnodes[i].partnerflags[r][k] points to allnodes[j].myflags[r][k]

    procedure dissemination_barrier
        for instance : integer :0 to LogP-1
	    localflags^.partnerflags[parity][instance]^ := sense
	    repeat until localflags^.myflags[parity][instance] = sense
	if parity = 1
	    sense := not sense
	parity := 1 - parity
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

	r = ceil_log2(num_threads);

	Stat = (MPI_Status*) malloc(num_threads * sizeof(MPI_Status));

}

void gtmpi_barrier(){

	int i, x, z, id, np, tag=1;

	MPI_Comm_size(MPI_COMM_WORLD, &np);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	for (i = 0; i < r; i++) {
		x = ((id + (power(2, i))) % np);
		MPI_Send(NULL, 0, MPI_INT, x, tag, MPI_COMM_WORLD);
		for (z = 0; z < np; z++) {
			if (id == (z + (power(2, i))) % np) {
				MPI_Recv(NULL, 0, MPI_INT, z, tag, MPI_COMM_WORLD, &Stat[z]);
			}
		}
	}

}

void gtmpi_finalize(){

	free(Stat);

}
