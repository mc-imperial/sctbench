#! /bin/bash
cd apps/barnes
./build.sh
cd ../..

cd kernels
cd fft
./build.sh
cd ..
cd lu/non_contiguous_blocks
./build.sh
cd ../..
cd ..


