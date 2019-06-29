#!/usr/bin/env python
# Run the benchmarks
from __future__ import print_function
from collections import OrderedDict, Counter
import os, sys, subprocess, shlex
import StringIO, datetime, argparse

cilkscreen = "/home/rob/src/cilktools-linux-004421/bin/cilkscreen"
use_cilkscreen = False
print_status = True # Makes log file ugly
column_size = 23
column_format = "{: >" + str(column_size) + "},"
status_column = 100

stat_lines = OrderedDict()
# stat_lines['insert_time'] =  'Total Insert time:'
stat_lines['query_time'] = 'Total Query time:'
# stat_lines['inserts'] = 'Num inserts:'
# stat_lines['relabels'] = 'Num relabels:'

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
    runtime = int(StringIO.StringIO(out).readline()[:-1])
    bufs = [StringIO.StringIO(out), StringIO.StringIO(err)]
    stats = Counter()
    for buf in bufs:
        for line in buf:
            for s in stat_lines.keys():
                ind = line.find(stat_lines[s])
                if ind > -1:
                    stats[s] = float(line[ind + len(stat_lines[s]):])
                    break
    return runtime, stats

def runit(n, bench_type, prog, args, env):
    cmd = prog + " " + args
    times = []
    stats = Counter()
    for i in range(n):
        proc = subprocess.Popen(cmd.split(), shell=False, env=env,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        t, results = parse_result(bench_type, proc)
        times.append(t)
        stats.update(results)
    for s in stats.keys():
        stats[s] = stats[s] / float(n)
    # output_string = "{0:10.2f} ({insert_time:10.2f}, {query_time:10.2f}, {inserts:4.0f}, {relabels:4.0f})"
    output_string = "{0:10.2f} ({query_time:10.2f})"
    return output_string.format(sum(times)/float(n), **stats)

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
    if use_cilkscreen:
        print(column_format.format("cilkscreen"), end='')
    for c in comp:
        print(column_format.format(c), end='')
    print()


def run_tests():
    num_iter = 3
    cores = [1] + range(2,17,2)
    runs = OrderedDict()
    #runs["matmul"] = "-n 32"
    runs["fft"] = "-n " + str(64*1024*1024)

    tests = runs.keys()
    args = runs.values()
    comp = ["brd"]
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
                line += result#column_format.format(result)
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
    run_tests()


if __name__ == "__main__":
    sys.exit(main())
