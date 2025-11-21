#!/bin/bash

mtx_arr=$(ls mtxs/*.mtx)

for MATRIX in $mtx_arr; do
    for THREADS in 1 8 16 32 64 96; do       
            gcc -o main main.c helpers.c -O3 -fopenmp -DTHREAD_NUMBER=$THREADS
            echo "Running with $THREADS threads on matrix $MATRIX"
            ./main $MATRIX
    done
done


