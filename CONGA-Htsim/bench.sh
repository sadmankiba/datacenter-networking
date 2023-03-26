#!/bin/bash

tms="10"
mf="-1"

while [[ -n $1 ]]; do
    case $1 in
        -t | --tms)   shift
            tms="$1"
            ;;
        -m | --mf)  shift 
            mf="$1"
    esac
    shift
done

echo  "Load%, n_ecmp, fct_ecmp, n_conga, fct_conga" > bench.csv
for load in 10 20 30 40 50 60 70 80 90 100; do
    echo -n "$load, " >> bench.csv
    ./htsim --expt=2 --ecmp=1 --log=0 --load=$load --tms=$tms --fs=10 --wl=0 --mf=$mf --of=0 >> bench.csv
    echo -n ", " >> bench.csv
    ./htsim --expt=2 --ecmp=0 --log=0 --load=$load --tms=$tms --fs=10 --wl=0 --mf=$mf --of=0 >> bench.csv
    echo "" >> bench.csv
done 