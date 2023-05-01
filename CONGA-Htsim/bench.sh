#!/bin/bash

tms="10"
mf="-1"
log="0"
fs=10

while [[ -n $1 ]]; do
    case $1 in
        -t | --tms)   shift
            tms="$1"
            ;;
        -m | --mf)  shift 
            mf="$1"
            ;;
        -l | --log) shift 
            log="$1"
            ;;
        -s | --fs) shift 
            fs="$1"
    esac
    shift
done

echo  "Load%, n_ecmp, fct_ecmp, n_conga, fct_conga" > bench.csv
for load in 10 20 30 40 50 60 70 80 90 100; do
    echo -n "$load, " >> bench.csv
    ./htsim --expt=2 --ecmp=1 --log=$log --load=$load --tms=$tms --fs=$fs --wl=0 --mf=$mf --of=0 >> bench.csv
    echo -n ", " >> bench.csv
    ./htsim --expt=2 --ecmp=0 --log=$log --load=$load --tms=$tms --fs=$fs --wl=0 --mf=$mf --of=0 >> bench.csv
    echo "" >> bench.csv
done 