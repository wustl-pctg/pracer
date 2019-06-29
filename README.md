# PRacer

This repository contains the PRacer race detection tools. PRacer is a
provably-good race detector for Cilk-P programs. PRacer uses a modified version
of the Cilk-P runtime system, so that code is also included.

## License:

The Cilk-P runtime system and our modifications to it are distributed under the
terms of the BSD-3-Clause license. The x264 and ferret benchmark are from PARSEC
benchmark suite and are distributed under the terms of the GNU General Public
License. All other code in this repository is distributed under the MIT license,
unless otherwise specified. 

## Dependencies:

A relatively recent version of Linux is recommended, with a C++11 version of the
C++ standard library. PRACER was tested on Linux kernel version 3.10.

## Using:

Run "./setup.sh" to install modified compiler and Cilk-P runtime system.

Use python script "run\_benchmark.py" to run benchmarks with certain configuration: 

    python run_benchmark.py bench_name cofiguration 

bench\_name should be one of the following: lz77, ferret, x264. configuration
should be one of the following: base, sp, full.
