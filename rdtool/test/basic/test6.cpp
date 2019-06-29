/* a cilk function that doesn't actually spawn. */

#include <assert.h>
#include <stdio.h>

#include "test.h"

void g(int n) {
    printf("hi\n");
}

int x=3;

void f(int n) {
    x++;
    if (n>1) {
	_Cilk_spawn g(n + x++);
    }
    if (n>2) {
	_Cilk_spawn g(n-1 +x++);
    }
}

int main(int argc, char *argv[] __attribute__((unused))) {
    f(argc);
    f(argc);
    //    assert(__cilksan_error_count() == 0);
    int num_races = get_num_races_found(); 
    assert(num_races == 0);

    
    return 0;
}
