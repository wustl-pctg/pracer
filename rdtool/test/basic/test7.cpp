// expect no race
#include <assert.h>
#include <stdio.h>

#include "test.h"

int touchn(int n) {
    int x[n];
    printf("&x[0]=%p\n", &x[0]);
    for (int i=0; i<n; i++) x[i] = n;
    int r = 0;
    for (int i=0; i<n-1; i++) {
	r += x[i]*x[i+1];
    }
    return r;
}

int foo() {
    int a = 0, b = 0;
    a = _Cilk_spawn touchn(2);
    b = _Cilk_spawn touchn(2);
    _Cilk_sync;
    return a + b;
}

int main() {
    int x = foo();
    assert(x!=0);
    //    assert(__cilksan_error_count() == 0);
    int num_races = get_num_races_found(); 
    assert(num_races == 0);
    
    return 0;
}
