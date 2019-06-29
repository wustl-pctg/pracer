#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cilk/cilk.h>
#include "bench.h"

/* 
 * nqueen  4 = 2 
 * nqueen  5 = 10 
 * nqueen  6 = 4 
 * nqueen  7 = 40 
 * nqueen  8 = 92 
 * nqueen  9 = 352 
 * nqueen 10 = 724
 * nqueen 11 = 2680 
 * nqueen 12 = 14200 
 * nqueen 13 = 73712 
 * nqueen 14 = 365596 
 * nqueen 15 = 2279184 
 */

// For batcher race detection, we only use a 4-byte granularity
#ifndef POSITION_TYPE
#define POSITION_TYPE int
#endif
typedef POSITION_TYPE pos_t;

/*
 * <a> contains array of <n> queen positions.  Returns 1
 * if none of the queens conflict, and returns 0 otherwise.
 */
int ok (int n, pos_t *a) {

  int i, j;
  pos_t p, q;

  for (i = 0; i < n; i++) {
    p = a[i];
    for (j = i + 1; j < n; j++) {
      q = a[j];
      if (q == p || q == p - (j - i) || q == p + (j - i))
        return 0;
    }
  }

  return 1;
}

int nqueens (int n, int j, pos_t *a) {

  pos_t *b;
  int i;
  int *count;
  int solNum = 0;

  if (n == j) {
    return 1;
  }

  count = (int *) alloca(n * sizeof(int));
  (void) memset(count, 0, n * sizeof (int));

  for (i = 0; i < n; i++) {

    /***
     * ANGE: strictly speaking, this (alloca after spawn) is frowned 
     * up on, but in this case, this is ok, because b returned by 
     * alloca is only used in this iteration; later spawns don't 
     * need to be able to access copies of b from previous iterations 
     ***/
    b = (pos_t *) alloca((j + 1) * sizeof (pos_t));
    memcpy(b, a, j * sizeof (pos_t));
    b[j] = i;

    if(ok (j + 1, b)) {
      count[i] = cilk_spawn nqueens(n, j + 1, b);
    }
  }
  cilk_sync; 

  for(i = 0; i < n; i++) {
    solNum += count[i];
  }

  return solNum;
}


int main(int argc, char *argv[]) { 

  int n = 13;
  pos_t *a;
  int res;

  if (argc < 2) {
    fprintf (stderr, "Usage: %s [<cilk-options>] <n>\n", argv[0]);
    fprintf (stderr, "Use default board size, n = 13.\n");

  } else {
    n = atoi (argv[1]);
    //printf ("Running %s with n = %d.\n", argv[0], n);
  }

  a = (pos_t *) alloca (n * sizeof (pos_t));
  res = 0;

  auto start = std::chrono::high_resolution_clock::now();
  res = nqueens(n, 0, a);
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

  if (res == 0) {
    printf ("No solution found.\n");
  } else {
    printf ("Total number of solutions : %d\n", res);
  }

  return 0;
}
