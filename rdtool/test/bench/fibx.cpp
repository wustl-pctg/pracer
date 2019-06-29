#include <cilk/cilk.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

int fibx(int n, int even_depth) {

  int x = 0, y = 0, _tmp = 0;

  if(n < 2 && n >= 0) {
    return n;
  } else if(n < 0) {
    return 0;
  }

  if(even_depth) {
    x = cilk_spawn fibx(n - 1, !even_depth); 
  } else {
    x = cilk_spawn fibx(n - 40, !even_depth); 
  }

  if(even_depth) {
    y = fibx(n - 40, !even_depth);
  } else {
    y = fibx(n - 1, !even_depth);
  }
  cilk_sync; 

  _tmp = x+y;

  return _tmp;
}

int main(int argc, char *argv[]) 
{
  int n, res;

  if(argc != 2) {
    fprintf(stderr, "Usage: fibx [<cilk-options>] <n>\n");
    exit(1);
  }
  n = atoi(argv[1]);

  auto start = std::chrono::high_resolution_clock::now();
  res = fibx(n, 1); 
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
  printf("Result: %d\n", res);

  return 0;
}
