# Usage example: ./bench-fio.sh <opts> 

BENCH_FILE=bench-fio.csv
FIO_OUT=fio.out
FILENAMES="/dev/nvme0n1"  
ENGS="libaio" # psync libaio xnvme
IOTYPES="randread" # randread read write
BLKS="128k" # 4k 128k
NJOBS=1
DEPTHS="4" # 1 4 32
TIME=10
SIZE=8G
CORE=5
FIXED=0

while [[ -n $1 ]]; do
    case $1 in
        -e | --eng)   shift
            ENGS="$1"
            ;;
        -f | --filenames)   shift
            FILENAMES="$1"
            ;;
        -i | --iotype)  shift 
            IOTYPES="$1"
            ;;
        -b | --block) shift
            BLKS="$1"
            ;;
        -n | --numjobs) shift
            NJOBS=$1
            ;;
        -q | --qdepth) shift
            DEPTHS="$1"
            ;;
        -t | --time) shift
            TIME=$1
            ;;
        -s | --size) shift
            SIZE=$1
            ;;
        -r | --core) shift
            CORE=$1
            ;;
        -x | --fixed) shift
            FIXED=$1
            ;;
        -h | --help) shift
            echo "Usage: ./bench-fio.sh [-e <engines>] [-f <filenames>] [-i <iotype>] [-b <blks>] [-n <jobs>] [-q <qdepths>]"
            echo "[-t <time>] [-s <size>] [-r <core>] [-h]"
            echo "Options:"
            echo "-e | --eng:   Engines to test. Options: psync libaio xnvme. Default: libaio"
            echo "-f | --filenames:   File to test. Default: /dev/nvme0n1"
            echo "-i | --iotype:  IO types to test. Options: randread read write. Default: randread"
            echo "-b | --block: Block sizes to test. Options: 4k 128k. Default: 128k"
            echo "-n | --numjobs: Number of jobs to run. Default: 1"
            echo "-q | --qdepth: Queue depths to test. Options: 1 4 32. Default: 4"
            echo "-t | --time: Time (s) to run each test. Default: 10"
            echo "-s | --size: Size of file. Default: 8G"
            echo "-r | --core: Start core to run fio on. Default: 5"
            echo "-x | --fixed: Fixed core <0 | 1>. Default: 0"
            exit 0
            ;;
    esac
    shift
done

# Parameters: $1 = filename
function bench_dev() {
    DEV=`echo $1 | awk 'BEGIN {FS="/"}; {printf("%s", $3)}'`
    FIO_OUT=fio-$DEV.out
    BENCH_FILE=bench-fio-$DEV.csv

    echo "IO Type, Block Size, IO Engine, Queue Depth, Num Jobs, Lat Avg (us), Lat Std (us), P99 Lat (us), IOPS (k), Bandwidth (MBps)" > $BENCH_FILE


    for IOTYPE in $IOTYPES; do
        for BLK_SIZE in $BLKS; do
            for IOENG in $ENGS; do
                for QDEPTH in $DEPTHS; do
                    echo -n "$IOTYPE, $BLK_SIZE, $IOENG, $QDEPTH, $NJOBS, " >> $BENCH_FILE
                    if [ $IOENG == xnvme ]; then
                        fio --name=job1 --rw=$IOTYPE --direct=1 --size=$SIZE --bs=$BLK_SIZE --ioengine=$IOENG --xnvme_dev_nsid=1 --thread=1 \
                            --iodepth=$QDEPTH --fsync=1 --numjobs=$NJOBS --filename=0000\\:22\\:00.0 --minimal --output=$FIO_OUT --time_based --runtime=${TIME}s
                    else 
                        taskset -c $CORE fio --name=job1 --rw=$IOTYPE --direct=1 --size=$SIZE --bs=$BLK_SIZE --ioengine=$IOENG --iodepth=$QDEPTH --fsync=1 \
                            --numjobs=$NJOBS --filename=$1 --minimal --output=$FIO_OUT --time_based --runtime=${TIME}s
                    fi
                    if [ $IOTYPE == write ]; then
                        awk 'BEGIN {FS=";"}; {printf("%.1f, %.1f, ", $81, $82)}' $FIO_OUT >> $BENCH_FILE
                        awk 'BEGIN {FS=";"}; {printf("%s", $69)}' $FIO_OUT | awk 'BEGIN {FS="="}; {printf("%.1f, ", $2)}' >> $BENCH_FILE
                        awk 'BEGIN {FS=";"}; {printf("%.1f, %.1f", $49/1000, $48/1000)}' $FIO_OUT >> $BENCH_FILE
                    else
                        # FIXME: needs to be updated for multiple jobs
                        awk 'BEGIN {FS=";"}; {printf("%.1f, %.1f, ", $40, $41)}' $FIO_OUT >> $BENCH_FILE
                        awk 'BEGIN {FS=";"}; {printf("%s", $28)}' $FIO_OUT | awk 'BEGIN {FS="="}; {printf("%.1f, ", $2)}' >> $BENCH_FILE
                        awk 'BEGIN {FS=";"}; {printf("%.1f, %.1f", $8/1000, $7/1000)}' $FIO_OUT >> $BENCH_FILE
                    fi
                    echo "" >> $BENCH_FILE
                done 
            done
        done 
    done
}

for FILENAME in $FILENAMES; do
    bench_dev $FILENAME &
    if [ $FIXED == 0 ]; then
        CORE=$(($CORE + 1))
    fi
done



