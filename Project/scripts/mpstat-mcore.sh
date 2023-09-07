CORE_START=5
COUNT=8

while [[ -n $1 ]]; do
    case $1 in
        -s | --start)  shift
            CORE_START=$1
            ;;
        -c | --count)  shift
            COUNT=$1
            ;;
        -h | --help) shift
            echo "Usage: ./mpstat-mcore.sh [-s <core_start>] [-c <count>] [-h]"
            echo "Options:"
            echo "-s | --start:   Core number to start from. Default: 5"
            echo "-c | --count:   Number of cores to monitor. Default: 8"
            exit 0
            ;;
    esac
    shift
done


for i in `seq 0 $(($COUNT - 1))`; do
    CORE=$(($CORE_START + $i))
    mpstat -P $CORE 1 > core-$CORE.out &
done
