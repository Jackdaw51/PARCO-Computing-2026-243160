#!/bin/bash

MATRIX="mtxs/fidapm11.mtx"
for THREADS in 1 8 16 32 64 96; do       
        gcc -o main main.c helpers.c -std=c99 -O3 -fopenmp -DTHREAD_NUMBER=$THREADS
        echo "Running with $THREADS threads on matrix $MATRIX"
        valgrind --tool=callgrind --cache-sim=yes --dump-instr=yes --collect-jumps=yes ./main $MATRIX
done



