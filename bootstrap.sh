#!/bin/bash
if NOT hash cmake 2>/dev/null; then
  echo "Unable to find cmake, did you forget to install it?"
  exit
fi

THREADS=9

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j${THREADS}
cmake ..
make -j${THREADS}
cmake -DBUILD_TESTING=ON ..
make -j${THREADS}
cmake ..
make -j${THREADS}
make test
cpack -G TGZ
