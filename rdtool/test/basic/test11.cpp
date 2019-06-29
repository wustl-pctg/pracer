// expect a race on heap-allocated data
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "test.h"

char x[16];

void foo(char *x) {
    x[0] = 1;
}

int main() {
    _Cilk_spawn foo(x);
    x[0] = 0;
    _Cilk_sync;
    //    assert(__cilksan_error_count() == 1);
    int num_races = get_num_races_found(); 
    assert(num_races == 1);


    return 0;
}
