// -*- C++ -*-

/*
 * qsort.cpp
 *
 * An implementation of quicksort using Intel(R) Cilk(TM) Plus parallelization.
 *
 * Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
 *
 * The source code contained or described herein and all
 * documents related to the source code ("Material") are owned by 
 * Intel Corporation or its suppliers or licensors. Title to the
 * Material remains with Intel Corporation or its suppliers and
 * licensors. The Material is protected by worldwide copyright
 * laws and treaty provisions.  No part of the Material may be
 * used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed,  or disclosed in any way
 * except as expressly provided in the license provided with the
 * Materials.  No license under any patent, copyright, trade
 * secret or other intellectual property right is granted to or
 * conferred upon you by disclosure or delivery of the Materials,
 * either expressly, by implication, inducement, estoppel or
 * otherwise, except as expressly provided in the license
 * provided with the Materials. 
 */

#include <cilk/cilk.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <functional>

#include <assert.h>
#include "bench.h"


// Sort the range between bidirectional iterators begin and end.
// end is one past the final element in the range.
// Use the Quick Sort algorithm, using recursive divide and conquer.
// This function is NOT the same as the Standard C Library qsort() function.
// This implementation is pure C++ code before Intel(R) Cilk(TM) Plus 
// conversion.
void sample_qsort(int * begin, int * end) {
  if (begin != end) {
    --end;  // Exclude last element (pivot) from partition
    int * middle = std::partition(begin, end,
        std::bind2nd(std::less<int>(), *end));
    using std::swap;
    swap(*end, *middle);    // move pivot to middle

    cilk_spawn sample_qsort(begin, middle);
    sample_qsort(++middle, ++end); // Exclude pivot and restore end
    cilk_sync;
  }
}

// A simple test harness 
int qmain(int n) {

  int* a = new int[n];

  for (int i = 0; i < n; ++i)
    a[i] = i;
  std::random_shuffle(a, a + n);

  //  std::cout << "Sorting " << n << " integers" << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  sample_qsort(a, a + n);
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

  // Confirm that a is sorted and that each element contains the index.
  for (int i = 0; i < n - 1; ++i) {
    if (a[i] >= a[i + 1] || a[i] != i) {
      std::cout << "Sort failed at location i=" << i << " a[i] = "
        << a[i] << " a[i+1] = " << a[i + 1] << std::endl;
      delete[] a;
      return 1;
    }
  }

  //  std::cout << "Sort succeeded." << std::endl;

  delete[] a;
  return 0;
}

int main(int argc, char* argv[]) {

  int n = 10 * 1000 * 1000;
  if (argc > 1) {
    n = std::atoi(argv[1]);
    if (n <= 0) {
      std::cerr << "Invalid argument" << std::endl;
      std::cerr << "Usage: qsort N" << std::endl;
      std::cerr << "       N = number of elements to sort" << std::endl;
      return 1;
    }
  }
  int ret = qmain(n);

  return ret;
}
