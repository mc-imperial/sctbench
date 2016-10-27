#!/bin/bash

source env.sh

# protobuf (download)
wget https://protobuf.googlecode.com/files/protobuf-2.4.1.tar.bz2
#extract
tar -xf protobuf-2.4.1.tar.bz2
#compile
cd protobuf-2.4.1
./configure --prefix=$PROTOBUF_HOME
make
#OPTIONAL: make check
#install
make install
cd ..

git clone git@bitbucket.org:paulthomson/maple.git maple
git clone git@bitbucket.org:paulthomson/pin.git pin

cd benchmarks
git clone git@bitbucket.org:paulthomson/parsec-splash.git splash2x
git clone git@bitbucket.org:paulthomson/parsecnative.git parsec-2.0
cd ..


# Build
cd maple
make compiletype=release
cd ..

cd benchmarks
cd parsec-2.0
./buildall.sh
cd ..
cd splash2x
./buildall.sh
cd ..
cd ..




