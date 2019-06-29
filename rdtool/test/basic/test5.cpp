// Expect no errors
#include <assert.h>
#include <stdio.h>

#include "test.h"

void add1(int n) {
    int a = 0;
    a += n;
    printf("add1(%d)\n", a);
}

void foo() {
    _Cilk_spawn add1(1);
    add1(3);
}

int main() {
    foo();
    //    assert(__cilksan_error_count() == 0);
    int num_races = get_num_races_found(); 
    assert(num_races == 0);

    assert(get_num_races_found() == 0);

    return 0;
}
