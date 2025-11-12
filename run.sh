#!/bin/bash

mtx_arr=$(ls mtxs/*.mtx)

for MATRIX in $mtx_arr; do
    for THREADS in 1 8 16 32 64; do
        for SCHEDULING in FOR STATIC DYNAMIC GUIDED; do
            if [ "$SCHEDULING" != "FOR" ]; then
                for CSIZE in 1 10 100 1000; do
                    gcc -o main main.c -fopenmp -DTHREAD_NUMBER=$THREADS -D$SCHEDULING -DCHUNKSIZE=$CSIZE 
                    echo "Running with $THREADS threads and $SCHEDULING scheduling (chunk size: $CSIZE) on matrix $MATRIX"
                    ./main $MATRIX
                done
                continue
            fi            
            gcc -o main main.c -fopenmp -DTHREAD_NUMBER=$THREADS -D$SCHEDULING
            echo "Running with $THREADS threads and $SCHEDULING scheduling on matrix $MATRIX"
            ./main $MATRIX
        done
    done
done