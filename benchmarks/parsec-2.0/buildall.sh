#!/bin/bash
source env.sh
parsecmgmt -a build -p streamcluster streamcluster2 streamcluster3 ferret
parsecmgmt -a run -p streamcluster streamcluster2 streamcluster3 ferret -s echo -n 2 -i test
#parsecmgmt -a build -p openmp -c gcc-openmp

