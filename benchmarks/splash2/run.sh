#! /bin/bash

case $1 in
barnes)
  cd apps/barnes
  ./run.sh
;;
fft)
  cd kernels/fft
  ./run.sh
;;
lu)
  cd kernels/lu/non_contiguous_blocks
  ./run.sh
;;
esac

