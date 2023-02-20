# 64/128/256/512/1K/2K/4K/8K/16K
# 1GB/2GB/4GB/8GB/16GB/32GB 

make clean
make

echo "Flow size, Latency (us), Bandwidth (Gbps)" > bench.csv
lat_sizes="64 128 256 512 1024 $((2 * 1024)) $((4 * 1024)) \
    $((8 * 1024)) $((16 * 1024))"
bw_sizes="$((1 * 1024 * 1024 * 1024)) $((2 * 1024 * 1024 * 1024)) \
    $((4 * 1024 * 1024 * 1024)) $((8 * 1024 * 1024 * 1024)) \
    $((16 * 1024 * 1024 * 1024)) $((32 * 1024 * 1024 * 1024))"

for sizes in "$lat_sizes"; do 
    for s in $sizes; do
        echo $s
        echo -n "$s, " >> bench.csv
        export FLOW_SIZE=$s 
        make run | tail -n 1 >> bench.csv
        echo "" >> bench.csv
        sleep 5
    done
done 