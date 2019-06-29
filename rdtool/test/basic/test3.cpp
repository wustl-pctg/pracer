// expect a race
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "test.h"

int a;

void add1(int n) {
    a++;
    printf("add1(%d)\n", n);
}

void foo(int x) {
    _Cilk_spawn add1(1);
    add1(x+2);
    _Cilk_sync;
    add1(3);
}

int main() {
    fprintf(stderr, "&a = %p.\n", &a);
    foo(0);

    // // See NOTES.txt
    // assert(__cilksan_error_count() >= 1 && __cilksan_error_count() <= 2);
    int num_races = get_num_races_found(); 
    assert(num_races >= 1 && num_races <= 2);


    return 0;
}
