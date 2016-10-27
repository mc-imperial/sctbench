#!/bin/bash

g++ -std=c++0x -g -O0 -o main.x SafeStack.cpp -pthread && \
rm -Rf run && \
mkdir run && \
cp main.x run
