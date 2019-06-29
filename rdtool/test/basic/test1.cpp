#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

// expect a race
#include "test.h"

#include <inttypes.h>

int a = 0;

void add1() {
    fprintf(stderr, "entering add1.\n");
    a++;
}

void bar() {
    fprintf(stderr, "entering bar.\n");
    _Cilk_spawn add1();
    _Cilk_sync;
    /*
    void *rsp, *rbp;
    asm ("movq %%rsp, %0;" : "=r" (rsp) : : );
    asm ("movq %%rbp, %0;" : "=r" (rbp) : : );
    fprintf(stderr, "bar rbp %p rsp %p.\n", rbp, rsp);
    */
}

void foo() {
    fprintf(stderr, "entering foo.\n");

    // _Cilk_spawn add1();
    _Cilk_spawn bar();
    fprintf(stderr, "  foo continue\n");
    add1();
    /*
    void *rsp, *rbp;
    asm ("movq %%rsp, %0;" : "=r" (rsp) : : );
    asm ("movq %%rbp, %0;" : "=r" (rbp) : : );
    fprintf(stderr, "foo rbp %p rsp %p.\n", rbp, rsp);
    */
}

int main() {
    fprintf(stderr, "&a = %p.\n", &a);
    fprintf(stderr, "calling foo.\n");
    foo();

    // See NOTES.txt 
    //    assert(__cilksan_error_count() >= 1 && __cilksan_error_count() <= 2);
    int num_races = get_num_races_found(); 
    assert(num_races >= 1 && num_races <= 2);

    
    return 0;
}
