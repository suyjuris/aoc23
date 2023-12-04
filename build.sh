#!/bin/bash

set -eu
shopt -s nullglob

CXX=g++

CXXFLAGS="-std=c++14 -fno-exceptions -fno-rtti"
CXXFLAGS_WARN="-Wall -Wextra -Wno-class-memaccess -Wno-sign-conversion -Wno-unused-variable -Wno-sign-compare -Wno-write-strings -Wno-unused-parameter -Wno-comment"
LDFLAGS="-lbtrfsutil -lcap"

NAME=aoc

# Default to g++ as we are using GCC extensions (clang works fine as well, though)
GXX=$(find /usr/bin -name 'g++*' | sort -t- -k2n | tail -n1)
CLANGXX=$(find /usr/bin -name 'clang++*' | sort -t- -k2n | tail -n1)
CXX=${CXX:-$GXX}
if [[ -z "$CXX" ]]; then
    echo "Error: no compiler found. Please specify a compiler by setting the CXX environment variable, i.e.:"
    echo "    CXX=clang++ $0 $@"
    exit 1
fi

if [[ "$#" > 0 && ( "$1" = "-h" || "$1" = "--help" ) ]]; then
    echo "Usage:"
    echo "  $0 [1|...|24]"
    exit 1
fi;

"$CXX" $CXXFLAGS $CXXFLAGS_WARN -O0 -ggdb -Werror "$1.cpp" -o "$NAME" $LDFLAGS
