#!/bin/bash

while [[ -n $1 ]]; do
    case $1 in
        -e | --eng)   shift
            ENGS="$1"
            ;;
        -f | --filename)   shift
            FILENAME=$1
            ;;
        -i | --iotype)  shift 
            IOTYPES="$1"
            ;;
    esac
    shift
done

echo  "Load%, n_ecmp, fct_ecmp, n_conga, fct_conga" > bench.csv
for load in 10 20 30 40 50 60 70 80 90 100; do
    echo -n "$load, " >> bench.csv
    ./htsim --expt=2 --ecmp=1 --log=0 --load=$load --tms=100 --fs=10 --wl=0 >> bench.csv
    echo -n ", " >> bench.csv
    ./htsim --expt=2 --ecmp=0 --log=0 --load=$load --tms=100 --fs=10 --wl=0 >> bench.csv
    echo "" >> bench.csv
done 