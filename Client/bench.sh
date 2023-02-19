# 64/128/256/512/1K/2K/4K/8K/16K
# 1GB/2GB/4GB/8GB/16GB/32GB 

make clean
make DEBUG=1

echo "Flow size, Latency (us), Bandwidth (Gbps)" > bench.out
sizes="64 128 256 512 1024 $((2 * 1024)) $((4 * 1024)) \
    $((8 * 1024)) $((16 * 1024))"
sms="64 128"

for s in $sizes; do
    echo $s
    echo -n "$s, " >> bench.out
    export FLOW_SIZE=$s 
    make run | tail -n 1 >> bench.out
    echo "" >> bench.out
    sleep 5
done