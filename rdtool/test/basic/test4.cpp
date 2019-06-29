// expect no error
#include <assert.h>
#include <stdio.h>

#include "test.h"

int a;

void add1(int n) {
    a++;
    printf("add1(%d)\n", n);
}

void foo() {
    _Cilk_spawn add1(1);
}

int main() {
    foo();

    //    assert(__cilksan_error_count() == 0);
    int num_races = get_num_races_found(); 
    assert(num_races == 0);

    return 0;
}
