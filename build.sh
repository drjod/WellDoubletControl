#!/bin/bash
module load cmake/3.15.4
module load intel/18.0.4

mkdir -p build
cd build
cmake .. -DCMAKE_CXX_COMPILER=icpc
make


#  mkdir -p build
#  cd build
#  cmake ..
#  make

