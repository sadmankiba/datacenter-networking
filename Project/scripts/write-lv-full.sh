
# Write to the full logical volume

COUNT=1
DEV=/dev/ubuntu-vg/test-lv
TIME=60

while [[ -n $1 ]]; do
    case $1 in
        -c | --count) shift
            COUNT=$1
            ;;
        -d | --device) shift
            DEV=$1
            ;;
        -t | --time) shift
            TIME=$1
            ;;
        -h | --help) shift
            echo "Usage: ./write-lv-full.sh [-c count] [-d device]"
            echo "  -c|--count <count>  : Number of LVs to write. Default: 1"
            echo "  -d|--device <device>  : Device to write to. e.g. /dev/nvme0n1p or /dev/ubuntu-vg/test-lv. Default: /dev/ubuntu-vg/test-lv"
            exit 0
            ;;
    esac
    shift
done 

for i in $(seq 0 $(($COUNT-1))); do
    echo "Writing to logical volume $i"
    ./bench-fio.sh -e libaio -f "$DEV$i" -n 32 -b 128k -q 4 -i write -s 70G -t $TIME &
done
