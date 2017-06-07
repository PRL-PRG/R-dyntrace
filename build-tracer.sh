#!/bin/bash

export CPPFLAGS="-g3 -O0 -ggdb"
export CFLAGS="-g3 -O0 -ggdb"
export R_KEEP_PKG_SOURCE=yes
export CXX="g++ -std=c++14"

cd rdt-plugins/promises &&
make clean &&
cmake . &&
make &&
cd ../.. 


