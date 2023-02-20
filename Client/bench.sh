# 64/128/256/512/1K/2K/4K/8K/16K
# 1GB/2GB/4GB/8GB/16GB/32GB 

make clean
make

lat_sizes="64 128 256 512 1024 $((2 * 1024)) $((4 * 1024)) \
    $((8 * 1024)) $((16 * 1024))"
bw_sizes="$((1 * 1024 * 1024 * 1024)) $((2 * 1024 * 1024 * 1024)) \
    $((4 * 1024 * 1024 * 1024)) $((8 * 1024 * 1024 * 1024)) \
    $((16 * 1024 * 1024 * 1024)) $((32 * 1024 * 1024 * 1024))"

if [ $1 == lat ]; then
    echo "Flow size, Latency (us), Bandwidth (Gbps)" > bench.csv
    for sizes in "$lat_sizes"; do 
        for s in $sizes; do
            echo $s
            echo -n "$s, " >> bench.csv
            export FLOW_NUM=1
            export FLOW_SIZE=$s 
            make run | tail -n 1 >> bench.csv
            echo "" >> bench.csv
            sleep 5
        done
    done
fi 

if [ $1 == flow ]; then 
    echo "Flow num, Latency (us), Bandwidth (Gbps)" > fn.csv
    for fn in 1 2 3 4 5 6 7 8; do
        echo -n "$fn, " >> fn.csv 
        export FLOW_NUM=$fn 
        export FLOW_SIZE=$((32 * 1024 * 1024))
        make run | tail -n 1 >> fn.csv 
        echo "" >> fn.csv 
        sleep 5
    done
fi 