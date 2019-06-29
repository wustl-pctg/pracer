import sys
import os
import re

work_dir = os.getcwd()

def x264(config):
    if config == "base":
        os.chdir(work_dir + "/piper-bench/x264/src")
        os.system("cp Makefile_ori Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("make clean > /dev/null 2>&1")
        os.system("make piper > /dev/null 2>&1")
        ret =  os.popen("./run_piper.sh")
        print "x264, baseline:"
        x264_extract_number(ret)

    if config == "sp":
        os.chdir(work_dir + "/rdtool")
        os.system("./copy.sh")
        os.chdir(work_dir + "/rdtool/src")
        os.system("cp instrumentation.cpp_om instrumentation.cpp")
        
        os.chdir(work_dir + "/piper-bench/x264/src")
        os.system("cp Makefile_om Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("./clean_rd.h > /dev/null 2>&1")
        os.system("make clean > /dev/null 2>&1")
        os.system("make piper > /dev/null 2>&1")
        ret =  os.popen("./run_piper.sh")
        print "x264, sp-maintenance:"
        x264_extract_number(ret)

        os.chdir(work_dir + "/rdtool")
        os.system("./revert.sh")

    if config == "full":
        os.chdir(work_dir + "/rdtool")
        os.system("./copy.sh")
        os.chdir(work_dir + "/rdtool/src")
        os.system("cp instrumentation.cpp_rd instrumentation.cpp")
        
        os.chdir(work_dir + "/piper-bench/x264/src")
        os.system("cp Makefile_rd Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("./clean_rd.h > /dev/null 2>&1")
        os.system("make clean > /dev/null 2>&1")
        os.system("make piper > /dev/null 2>&1")
        ret =  os.popen("./run_piper.sh")
        print "x264, full:"
        x264_extract_number(ret)

        os.chdir(work_dir + "/rdtool")
        os.system("./revert.sh")


def x264_extract_number(handle):
    i = 1
    line = handle.readline()
    while line:
        number = re.search(r"Running time:\s+(([0-9]|\.)+)", line)
        if number:
            print "cores used: ", i , "elapsed time in second: ", number.group(1)
            if i < 4:
                i = i * 2
            else:
                i = i + 4
        line = handle.readline()

    handle.close()


def lz77(config):
    if config == "base":
        os.chdir(work_dir + "/piper-bench/lz77")
        os.system("cp Makefile_ori Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("make clean > /dev/null 2>&1")
        os.system("make > /dev/null 2>&1")
        ret =  os.popen("./run.sh")
        print "lz77, baseline:"
        lz77_extract_number(ret)

    if config == "sp":
        os.chdir(work_dir + "/rdtool")
        os.system("./copy.sh")
        os.chdir(work_dir + "/rdtool/src")
        os.system("cp instrumentation.cpp_om instrumentation.cpp")
        
        os.chdir(work_dir + "/piper-bench/lz77")
        os.system("cp Makefile_om Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("./clean_rd.h > /dev/null 2>&1")
        os.system("make clean > /dev/null 2>&1")
        os.system("make > /dev/null 2>&1")
        ret =  os.popen("./run.sh")
        print "lz77, sp-maintenance:"
        lz77_extract_number(ret)

        os.chdir(work_dir + "/rdtool")
        os.system("./revert.sh")

    if config == "full":
        os.chdir(work_dir + "/rdtool")
        os.system("./copy.sh")
        os.chdir(work_dir + "/rdtool/src")
        os.system("cp instrumentation.cpp_rd instrumentation.cpp")
        
        os.chdir(work_dir + "/piper-bench/lz77")
        os.system("cp Makefile_rd Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("./clean_rd.h > /dev/null 2>&1")
        os.system("make clean > /dev/null 2>&1")
        os.system("make > /dev/null 2>&1")
        ret =  os.popen("./run.sh")
        print "lz77, full:"
        lz77_extract_number(ret)

        os.chdir(work_dir + "/rdtool")
        os.system("./revert.sh")


def lz77_extract_number(handle):
    i = 1
    line = handle.readline()
    while line:
        number = re.search(r"Elapsed time in second:\s+(([0-9]|\.)+)", line)
        if number:
            print "cores used: ", i , "elapsed time in second: ", number.group(1)
            if i < 4:
                i = i * 2
            else:
                i = i + 4
        line = handle.readline()

    handle.close()

def ferret(config):
    if config == "base":
        os.chdir(work_dir + "/piper-bench/ferret/src")
        os.system("cp Makefile_ori Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("make clean > /dev/null 2>&1")
        os.system("make full_clean > /dev/null 2>&1")
        os.system("make piper > /dev/null 2>&1")
        os.chdir(work_dir + "/piper-bench/ferret")
        ret =  os.popen("./run_piper_native.sh")
        print "ferret, baseline:"
        ferret_extract_number(ret)

    if config == "sp":
        os.chdir(work_dir + "/rdtool")
        os.system("./copy.sh")
        os.chdir(work_dir + "/rdtool/src")
        os.system("cp instrumentation.cpp_om instrumentation.cpp")
        
        os.chdir(work_dir + "/piper-bench/ferret/src")
        os.system("cp Makefile_om Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("./clean_rd.h > /dev/null 2>&1")
        os.system("make clean > /dev/null 2>&1")
        os.system("make full_clean > /dev/null 2>&1")
        os.system("make piper > /dev/null 2>&1")
        os.chdir(work_dir + "/piper-bench/ferret")
        ret =  os.popen("./run_piper_native.sh")
        print "ferret, sp-maintenance:"
        ferret_extract_number(ret)

        os.chdir(work_dir + "/rdtool")
        os.system("./revert.sh")

    if config == "full":
        os.chdir(work_dir + "/rdtool")
        os.system("./copy.sh")
        os.chdir(work_dir + "/rdtool/src")
        os.system("cp instrumentation.cpp_rd instrumentation.cpp")
        
        os.chdir(work_dir + "/piper-bench/ferret/src")
        os.system("cp Makefile_rd Makefile")
        os.system("sed -i \'s/WORK_DIR/" + work_dir.replace("/", "\\/") + "/\' Makefile")
        os.system("./clean_rd.h > /dev/null 2>&1")
        os.system("make clean > /dev/null 2>&1")
        os.system("make full_clean > /dev/null 2>&1")
        os.system("make piper > /dev/null 2>&1")
        os.chdir(work_dir + "/piper-bench/ferret")
        ret =  os.popen("./run_piper_native.sh")
        print "ferret, full:"
        ferret_extract_number(ret)

        os.chdir(work_dir + "/rdtool")
        os.system("./revert.sh")


def ferret_extract_number(handle):
    i = 1
    line = handle.readline()
    while line:
        number = re.search(r"QUERY TIME:\s+(([0-9]|\.)+)", line)
        if number:
            print "cores used: ", i , "elapsed time in second: ", number.group(1)
            if i < 4:
                i = i * 2
            else:
                i = i + 4
        line = handle.readline()

    handle.close()


def run(bench_name, config):
    if bench_name == "lz77":
        lz77(config)

    if bench_name == "ferret":
        ferret(config)

    if bench_name == "x264":
        x264(config)
        

def check_argv(bench_name, config):
    if bench_name != "lz77" and bench_name != "ferret" and bench_name != "x264":
            return False

    if config != "base" and config != "sp" and config != "full":
            return False

    return True

if __name__ == "__main__":
    if len(sys.argv) != 3 or not check_argv(sys.argv[1], sys.argv[2]):
        print "USAGE: python run_benchmark.py bench_name, configuration.\nbench_name should be one of the following: lz77, ferret, x264.\nconfiguration should be one of the following: base, sp, full."
        exit(0)

    run(sys.argv[1], sys.argv[2])
