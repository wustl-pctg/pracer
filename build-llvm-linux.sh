#!/bin/bash

###### This file keeps the script to build Linux version of CilkPlus Clang based compiler

###### By default we assume you're using 'llvm-cilk' folder created in the current directory as a base folder of your source installation
###### If you use some specific location then pass it as argument to this script

if [ "$1" = "" ]
then
  LLVM_NAME=llvm-cilk
else
  LLVM_NAME=$1
fi

: ${BINUTILS_PLUGIN_DIR:="/usr/local/include"}

LLVM_HOME=`pwd`/"$LLVM_NAME"/src
LLVM_TOP=`pwd`/"$LLVM_NAME"

LLVM_GIT_REPO="https://gitlab.com/wustl-pctg-pub/llvm-cilk.git"
LLVM_BRANCH="cilkplus"
CLANG_GIT_REPO="https://gitlab.com/wustl-pctg-pub/clang-cilk.git"
CLANG_BRANCH="compressed_pedigrees"
# We will use the master branch instead of Cilk Plus branch; we want to
# install our own runtime instead of using theirs
COMPILERRT_GIT_REPO="https://github.com/cilkplus/compiler-rt"
COMPILERRT_BRANCH=""

echo Building $LLVM_HOME...

if [ ! -d $LLVM_HOME ]; then
    if [ "" != "$LLVM_BRANCH" ]; then
        git clone -b $LLVM_BRANCH $LLVM_GIT_REPO $LLVM_HOME
    else
        git clone $LLVM_GIT_REPO $LLVM_HOME
    fi
else
    cd $LLVM_HOME
    git pull --rebase
    cd -
fi

if [ ! -d $LLVM_HOME/tools/clang ]; then
    if [ "" != "$CLANG_BRANCH" ]; then
        git clone -b $CLANG_BRANCH $CLANG_GIT_REPO $LLVM_HOME/tools/clang
    else
        git clone $CLANG_GIT_REPO $LLVM_HOME/tools/clang
    fi
else
    cd $LLVM_HOME/tools/clang
    git pull --rebase
    cd -
fi

#rm -rf $LLVM_HOME/projects/compiler-rt
#if [ "" != "$COMPILERRT_BRANCH" ]; then
#    git clone -b $COMPILERRT_BRANCH $COMPILERRT_GIT_REPO $LLVM_HOME/projects/compiler-rt
#else
#    git clone $COMPILERRT_GIT_REPO $LLVM_HOME/projects/compiler-rt
#fi

#cd $LLVM_HOME/projects/compiler-rt
#git checkout b696762
#cd -

BUILD_HOME=$LLVM_HOME/build
if [ ! -d $BUILD_HOME ]; then
    mkdir -p $BUILD_HOME
fi
cd $BUILD_HOME

set -e
echo ../configure --prefix="$LLVM_TOP"

if [[ ($BINUTILS_PLUGIN_DIR != "") && (-e $BINUTILS_PLUGIN_DIR/plugin-api.h) ]]; then
    echo "Using bintuils gold header: $BINUTILS_PLUGIN_DIR/plugin-api.h"
    ../configure --prefix="$LLVM_TOP" --enable-targets=host --enable-optimized --with-binutils-include="$BINUTILS_PLUGIN_DIR"
else
    echo "NOT using bintuils gold header." 
    ../configure --prefix="$LLVM_TOP" --enable-targets=host --enable-optimized
fi

# ###### Now you're able to build the compiler
physicalCpuCount=$([[ $(uname) = 'Darwin' ]] &&
    sysctl -n hw.physicalcpu_max ||
    lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)
make -j$physicalCpuCount > build.log
make install

###### Produce a shell script, "usellvm.sh", to set up environment to
###### use llvm-cilk-ok.
# echo export PATH=$LLVM_TOP/bin:'$PATH' > $LLVM_TOP/usellvm.sh
# echo export LIBRARY_PATH=$LLVM_TOP/lib:'$LIBRARY_PATH' >> $LLVM_TOP/usellvm.sh
# echo export LD_LIBRARY_PATH=$LLVM_TOP/lib:'$LD_LIBRARY_PATH' >> $LLVM_TOP/usellvm.sh
# echo export C_INCLUDE_PATH=$LLVM_TOP/include:'$CPATH' >> $LLVM_TOP/usellvm.sh
# echo export CPLUS_INCLUDE_PATH=$LLVM_TOP/include:'$CPATH' >> $LLVM_TOP/usellvm.sh

###### That's it!  Source the usellvm.sh script generated by this file
###### to set up your environment to use this compilation of LLVM.
