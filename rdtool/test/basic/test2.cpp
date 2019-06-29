// expect a race
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "test.h"

int a;

void add1(int n) {
    a++;
    if (0) printf("add1(%d)\n", n);
}

void foo() {
    add1(0);
    _Cilk_spawn add1(1);
    add1(2);
    _Cilk_spawn add1(3);
    add1(4);
    _Cilk_sync;
    add1(5);
}

int main() {
    printf("  &a=%p\n", &a);
    foo();

    // See NOTES.txt
    //    assert(__cilksan_error_count() >= 1 && __cilksan_error_count() <= 2);
    int num_races = get_num_races_found(); 
    assert(num_races >= 1 && num_races <= 2);


    return 0;
}
