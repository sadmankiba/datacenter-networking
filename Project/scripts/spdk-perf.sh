#
# A script to run spdk perf app from NVMe initiator
# 

OUT_FILE=spdk-perf.out
CSV_FILE=spdk-perf.csv
TR_TYPE=TCP         # local | RDMA | TCP
TR_ADDR=192.168.50.7    # target address
IOTYPES="read"
BLK_SIZES="131072"  # 4096 131072
DEPTHS="4"
TIME=10
CORE_MASK=0x20

while [[ -n $1 ]]; do
    case $1 in
        -t | --tr)   shift
            TR_TYPE=$1
            ;;
        -a | --addr) shift
            TR_ADDR=$1
            ;;
        -i | --iotype) shift
            IOTYPES="$1"
            ;;
        -b | --blksize) shift
            BLK_SIZES="$1"
            ;;
        -c | --core) shift
            CORE_MASK=$1
            ;;
        -m | --time) shift
            TIME=$1
            ;;
        -d | --depth) shift
            DEPTHS="$1"
            ;;
        -h | --help) shift
            echo "Usage: ./spdk-perf.sh -t <TR_TYPE> -a <ADDR> -i <IOTYPES> -b <BLK_SIZES> -m <TIME>"
            echo "-t | --tr:  Transport type. Options: local | RDMA | TCP. Default: TCP"
            echo "-a | --addr: target address. Default: 192.168.50.7"
            echo "-i | --iotype:  IO types to test. Options: randread read randwrite write. Default: read"
            echo "-b | --blksize: Block sizes to test. Options: 4096(4k), 131072 (128k) etc. Default: 131072"
            echo "-c | --core: Core mask to run spdk perf app. Default: 0x20"
            echo "-d | --depth: Queue depths to test. Default: 4"
            echo "-m | --time: Time (s) to run each test. Default: 10"
            exit 0
            ;;
    esac
    shift
done

echo "IO Type, Block size, Queue depth, Lat Avg (us), Lat Min-Max (us), IOPS (k), Bandwidth (MiB/s)" > $CSV_FILE

for iotype in $IOTYPES; do 
    for blk_size in $BLK_SIZES; do 
        for dep in $DEPTHS; do 
            echo -n "$iotype, $blk_size, $dep, " >> $CSV_FILE
            if [ $TR_TYPE == "local" ]; then
                spdk_nvme_perf -c $CORE_MASK -p 8 -q $dep -o $blk_size -w $iotype \
                    -r 'trtype:PCIe traddr:0000:22:00.0' -t $TIME > $OUT_FILE
            else
                spdk_nvme_perf -c $CORE_MASK -P 8 -q $dep -o $blk_size -w $iotype \
                    -r "trtype:$TR_TYPE adrfam:IPv4 traddr:$TR_ADDR trsvcid:4420" -t $TIME > $OUT_FILE
            fi
            awk '/^Total/ {printf("%.0f, [%.0f-%.0f], %.1f, %.0f", $5, $6, $7, $3/1000, $4)}' $OUT_FILE >> $CSV_FILE
            echo "" >> $CSV_FILE
        done
    done
done 

rm $OUT_FILE