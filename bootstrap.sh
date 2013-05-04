#!/bin/bash
if NOT hash cmake 2>/dev/null; then
  echo "Unable to find cmake, did you forget to install it?"
  exit
fi

rm -rf build
mkdir build
cd build
cmake ..
make -j
cmake ..
make -j
cmake -DBUILD_TESTING=ON ..
make -j
cmake ..
make -j
make install
cpack -G TGZ
