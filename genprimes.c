#include "mpi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isprime(int num) {
  int i, sqroot;
  if (num >= 1) {
    sqroot = (int)sqrt(num);
    for (i = 3; i <= sqroot; i = i + 2)
      if ((num % i) == 0)
        return 0;
    return 1;
  } else {
    return 0;
  }
}

typedef struct {
  int *buffer;
} PrimeBuffer;

int *gen_primes(int start, int N, int stride) {
  int *buffer = malloc((N - start + 1) * sizeof(int));
  buffer[0] = 0;
  for (int i = start; i <= N; i = i + stride) {
    if (isprime(i)) {
      int count = ++buffer[0];
      if(i==1){
      buffer[count]=2;
      }
      else
       buffer[count] = i;
    }
  }
  return buffer;
}

int main(int argc, char *argv[]) {
  int procs,
      rank,
      primes,
      start,
      stride;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &procs);

  int N = atoi(argv[1]);
  double start_time, end_time;

  start_time = MPI_Wtime();
  start = (rank * 2) + 1;
  stride = procs * 2;

  int *buffer = gen_primes(start, N, stride);
  int count = buffer[0];
  MPI_Reduce(&count, &primes, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Bcast(&primes, 1, MPI_INT, 0, MPI_COMM_WORLD);

  int final_buffer_size = procs * (primes + 1);
  int *final_buffer;
  if (rank == 0)
    final_buffer = malloc(final_buffer_size * sizeof(int));

  MPI_Gather(buffer, 1 + primes, MPI_INT, final_buffer, primes + 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    int begin = 0;
    for (int i = 0; i < final_buffer_size; i += primes + 1) {
      int count = final_buffer[i];
      for (int j = 0; j < count; j++) {
        final_buffer[begin++] = final_buffer[j + i + 1];
      }
    }

    printf("Processes: %d, Numbers: %d\n", procs, N);

    char src[256];
    sprintf(src, "%s.txt", argv[1]);
    FILE *fp = fopen(src, "w");
    fprintf(fp, "%d", final_buffer[0]);

    for (int i = 1; i < begin; i++)
      fprintf(fp, " %d", final_buffer[i]);

    end_time = MPI_Wtime();

    printf("Total primes: %d\n", begin);
    printf("Elapsed time = %e seconds\n", end_time - start_time);
    fclose(fp);
  }

  MPI_Finalize();
  return 0;
}