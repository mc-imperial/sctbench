#!/bin/bash

cd bzip2-1.0.6 && \
make libbz2.a && \
cd .. && \
cd pbzip2-0.9.4 && \
make && \
cd .. && \
rm -Rf run && \
mkdir run && \
cp pbzip2-0.9.4/pbzip2 run && \
cp test.tar run

