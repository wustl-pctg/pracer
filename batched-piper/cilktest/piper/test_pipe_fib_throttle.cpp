/*  test_pipe_fib_throttle.cpp                  -*- C++ -*-
 *
 *  @copyright
 *  Copyright (C) 2013, Intel Corporation
 *  All rights reserved.
 *  
 *  @copyright
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *    * Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *  
 *  @copyright
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 *  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file test_pipe_fib_throttle.cpp
 *
 * @brief Driver for pipe_fib, run with different throttling
 * parameters.
 */


// All the interesting stuff is shared in a common .h file.
#include "pipe_fib_common.h"


int main_wrapper(int argv, char *args[]) {
#define MAX(x, y) (x ^ ((x ^ y) & -(x < y)))

    double tm_begin;
    double tm_elapsed[TIMES_TO_RUN];
    
    long max_bits = 0;
    int n;
    if (argv <= 1) {
        CILKTEST_PRINTF(1, "Usage: %s <n> [max_bits].\n", args[0]);
        n = 3000;
    }
    else {
        n = atoi( args[1] );
    }
    
    if (argv > 2) {
        max_bits = atoi( args[2] );
    } else {   
        max_bits = ceil(n * log2(1.62));
        max_bits = MAX(64, max_bits);
    }
    
    uint64_t res = 0;
    int length = max_bits + 1;
    char bStr[length];

    if (max_bits <= 64) {
        CILKTEST_PRINTF(2, "Computing fib(%d) using %ld bits.\n", n, max_bits); 
        res = linear_fib(n);
        intToBinaryString(bStr, length, res);
        CILKTEST_PRINTF(2, "Fib of %d is %10llu.\n", n, res);
        CILKTEST_PRINTF(2, "hex: 0x%llx \n", res);
        CILKTEST_PRINTF(2, "binary: \n%s \n", bStr);
    } else {
        CILKTEST_PRINTF(2, "Computing fib(%d) using %ld bits.\n", n, max_bits); 
    }

    // fib contains the binary representation of fibbonacci 
    // fib[*][0] always contains the width of the binary representation
    // fib[i%3][j] contains the jth digits (0 or 1) for fib(i)
    BitVal *fib[3];
    BitVal *serial_fib[3];
    int array_size = sizeof(BitVal) * (max_bits + 1);

    if (CHECK_RESULT) {
        serial_fib[0] = (BitVal *) alloca( array_size );
        serial_fib[1] = (BitVal *) alloca( array_size );
        serial_fib[2] = (BitVal *) alloca( array_size );
        compute_serial_fib(serial_fib, array_size, n);
        if (0) {
            for (int j = 0; j < max_bits+1; ++j) {
                CILKTEST_PRINTF(2,
                                "serial_fib[0][%d] = %d, serial_fib[1][%d] = %d, serial_fib[2][%d] = %d\n",
                                j, serial_fib[0][j],
                                j, serial_fib[1][j],
                                j, serial_fib[2][j]);
            }
        }
    }
    
    fib[0] = (BitVal *) alloca( array_size );
    fib[1] = (BitVal *) alloca( array_size );
    fib[2] = (BitVal *) alloca( array_size );

    // Test various throttling sizes.
    for (int K = 2; K < 25; ++K) {
        for(int i = 0; i < TIMES_TO_RUN; i++) {
            // re-initialize the fib array
            memset(fib[0], 0, array_size);
            memset(fib[1], 0, array_size);
            memset(fib[2], 0, array_size);

            tm_begin = Cilk_get_wall_time();
            pipe_fib_with_throttling(n, fib, K);
            tm_elapsed[i] = Cilk_get_wall_time() - tm_begin;

            if (CHECK_RESULT) {
                int num_errors = 0;
                if (0) {
                    for (int j = 0; j < array_size; ++j) {
                        CILKTEST_PRINTF(3,
                                        "fib[0][%d] = %d, fib[1][%d] = %d, fib[2][%d] = %d\n",
                                        j, fib[0][j],
                                        j, fib[1][j],
                                        j, fib[2][j]);
                    }
                }
                for (int j = 0;  j < max_bits+1; ++j) {
                    TEST_ASSERT(serial_fib[n%3][j] == fib[n%3][j]);

                    if (serial_fib[n%3][j] != fib[n%3][j]) {
                        CILKTEST_PRINTF(0,
                                        "Error %d: bit %d expected %d, computed = %d\n",
                                        num_errors,
                                        j,
                                        serial_fib[n%3][j],
                                        fib[n%3][j]);
                    }
                }
            }
            
            if(max_bits <= 64) {
                TEST_ASSERT(check_result(fib[n%3], bStr, array_size));
            }
        }
    
        double avg_time = 0.0;
        int P = __cilkrts_get_nworkers();
        for (int i = 0; i < TIMES_TO_RUN; ++i) {
            CILKTEST_PRINTF(3,
                            "P = %d, run %d, time = %g ms\n",
                            P, i, tm_elapsed[i]);
            avg_time += tm_elapsed[i];
        }
        avg_time /= TIMES_TO_RUN;
    
        CILKTEST_PRINTF(2,
                        "P=%d, K = %d, avg time = %g ms\n",
                        P, K, avg_time);

    }
    return 0;
}

int main(int argc, char* argv[]) {
    CILKTEST_BEGIN("pipe_fib_throttle");
    main_wrapper(argc, argv);
    CILKTEST_END("pipe_fib_throttle");
    return 0;
}

