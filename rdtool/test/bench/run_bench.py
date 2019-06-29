#!/usr/bin/env python
# Run the benchmarks
from __future__ import print_function
from collections import OrderedDict
import os, sys, shlex, subprocess
import StringIO, datetime, argparse

## @todo In the future, print out to a log file, by default named with a timestamp, e.g.
## shell: date +%Y%m%d-%H%M
## python: import datetime; datetime.datetime.now().strftime("%Y%m%d-%H%M")

cilkscreen = "/home/rob/src/cilktools-linux-004421/bin/cilkscreen"
use_cilkscreen = False
calc_stdev = True
print_status = True # Makes log file ugly
column_size = 22
if calc_stdev: column_size += 6
column_format = "{: >" + str(column_size) + "},"
status_column = 100

def mean(data):
    """Return the sample arithmetic mean of data."""
    n = len(data)
    if n < 1:
        raise ValueError('mean requires at least one data point')
    return sum(data)/float(n)

def _ss(data):
    """Return sum of square deviations of sequence data."""
    c = mean(data)
    ss = sum((x-c)**2 for x in data)
    return ss

def pstdev(data):
    """Calculates the population standard deviation."""
    n = len(data)
    if n < 2:
        raise ValueError('variance requires at least two data points')
    ss = _ss(data)
    pvar = ss/n # the population variance
    return pvar**0.5

def timestamp():
    return datetime.datetime.now()

# For now, returns just the runtime, in a list
def parse_result(bench_type, proc):
    out, err = proc.communicate()
    if proc.returncode != 0:
        print("\nExecution terminated with:")
        sep = "-"*column_size
        print(sep + "stdout" + sep)
        print(out)
        print(sep + "stderr" + sep)
        print(err)
        sys.exit(1)
    buf = StringIO.StringIO(out)
    line = buf.readline()
    return int(line[:-1])

def runit(n, bench_type, prog, args, env):
    cmd = prog + " " + args
    results = []
    for i in range(n):
        proc = subprocess.Popen(cmd.split(), shell=False, env=env,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        results.append(parse_result(bench_type, proc))
    s = "{0:.2f}".format(mean(results))
    if calc_stdev:
        s += " ({0:.2f})".format(pstdev(results))
    return s
    #return "{0:.2f}, {1:.2f}".format(mean(results),pstdev(results))

def pre_status(prev_line):
    if not print_status: return
    s = "Started latest at {:%d %b %Y %H:%M}".format(timestamp())
    s = s.rjust(status_column - len(prev_line))
    sys.stderr.write(s)
    sys.stderr.flush()    

def post_status(new_line):
    if print_status:
        sys.stderr.write('\r')
        sys.stderr.flush()
        sys.stdout.write('\r')
        sys.stdout.flush()
    print(new_line, end='')
    sys.stdout.flush()

def print_header(comp):
    commit = subprocess.check_output("git rev-parse HEAD".split(), shell=False)[0:-1]
    print("Running benchmarks with tool built from {}".format(commit))
    print("P", end='')
    for c in comp:
        print(column_format.format(c), end='')
    if use_cilkscreen:
        print(column_format.format("cilkscreen"), end='')
    print()


def run_tests():
    num_iter = 3
    cores = [1] + range(2,17,2)

    runs = OrderedDict()
    #runs["matmul"] = "-n 2048"
    #runs["cilksort"] = "-n 100000000"
    #runs["heat"] = "-nx 2048 -ny 2048 -nt 500"
    #runs["fft"] = "-n " + str(64*1024*1024)
    #runs["cholesky"] = "-n 3000 -z 30000"
    #runs["fib"] = "35"
    #runs["nqueens"] = "13" ## @bug Detects race!
    #runs["fibx"] = "772"

    # runs["qsort"] = "60000000"
    # runs["knapsack"] = "-f knapsack-example4.input"
    # runs["rectmul"] = "-x 3500 -y 3500 -z 3500"
    # runs["strassen"] = "-n 2048"
    # runs["lu"] = "-n 2048"

    # runs["strassen"] = "-n 2048"
    # runs["lu"] = "-n 2048"
    runs["qsort"] = "50000000"
    runs["rectmul"] = "-x 2500 -y 2500 -z 2500"
    runs["knapsack"] = "-f knapsack-example4.input"
    
    tests = runs.keys()
    args = runs.values()
    comp = ["base", "insert", "brd", "cilksan"]
    #comp = ["cilksan"]
    bin_dir = "bin"
    global status_column
    status_column = (len(comp)+1) * (column_size+3) + 25
    if use_cilkscreen: status_column += column_size

    # Let's validate the benchmarks so we don't waste time
    for i in range(0, len(tests)):
        base = os.path.join(os.getcwd(), bin_dir, tests[i])
        for c in comp:
            prog = base + "_" + c
            if not os.path.isfile(prog):
                print("Error: {} does not exist!".format(prog))
                sys.exit(1)
    
    print_header(comp)

    for i in range(0,len(tests)):
        base = tests[i]
        print("------------------------------")
        print("Running {} {} times with '{}':".format(base,num_iter,args[i]))
        sys.stdout.flush()
        base = os.path.join(os.getcwd(), bin_dir, base)
        cilkscreen_result = ""
        cilksan_result = ""

        for p in cores:
            env = dict(os.environ, CILK_NWORKERS=str(p))
            line = "{:2},".format(p)
            print(line, end='')
            sys.stdout.flush()

            for c in comp:
                prog = base + "_" + c
                if not print_status: line = ""
                pre_status(line)
                if c == "cilksan" and p != 1:
                    if not print_status: line = ""
                    line += column_format.format(cilksan_result)
                    post_status(line)
                    continue
                result = runit(num_iter, c, prog, args[i], env)
                if c == "cilksan" and p == 1: cilksan_result = result
                line += column_format.format(result)
                post_status(line)

            if use_cilkscreen:
                if not print_status: line = ""
                pre_status(line)
                if p == 1:
                    prog = cilkscreen + " -- " + base + "_icc"
                    cilkscreen_result = runit(num_iter, "cilkscreen", prog, args[i], env)
                line += column_format.format(cilkscreen_result)
                post_status(line)

            if print_status:
                s = " " * 20
                s = s.rjust(status_column - len(line))
            else:
                s = ""
            print(s)
            sys.stdout.flush()    

def main(argv=None):
    if sys.version_info < (2,7):
        print("This script requires Python 2.7+")
        sys.exit(1)
    parser = argparse.ArgumentParser(description='Run the benchmarks')
    parser.add_argument('--log', dest='log_file', 
                        help='Output to a log file. (default: ???)')
    run_tests()

if __name__ == "__main__":
    sys.exit(main())
