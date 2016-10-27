#!/bin/bash


cd run && \
runner ./pbzip2 -k -f -p$num_threads -1 -b1 test.tar

