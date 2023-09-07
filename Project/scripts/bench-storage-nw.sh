CORE=5
TIME=20
COUNT_ST=1
COUNT_NW=1
L_APP=0

while [[ -n $1 ]]; do
    case $1 in
        -t | --time)   shift
            TIME=$1
            ;;
        -l | --l_app)   shift
            L_APP=$1
            ;;
        -s | --count_st)   shift
            COUNT_ST=$1
            ;;
        -n | --count_nw)   shift
            COUNT_NW=$1
            ;;
        -h | --help) shift
            echo "Usage: $0 [-t|--time <time>]"
            echo "  -t|--time <time>  : Time (s) to run each test. Default: 20"
            echo "  -s|--count_st <count>  : Number of storage apps to run. Default: 1"
            echo "  -n|--count_nw <count>  : Number of network apps to run. Default: 1"
            echo "  -l|--l_app <0|1>  : Run l-apps. Default: 0"
            exit 0
            ;;
    esac
    shift
done

FILES=""
for ((i=0; i<$COUNT_ST; i++)); do
    FILES="$FILES /dev/nvme1n$(($i+1))"
done 

set -x
if [ $L_APP -eq 1 ]; then
    ./bench-fio.sh -f "$FILES" -t $TIME -r $CORE -b 4k -q 1 -i randread -x 1 & 
else 
    ./bench-fio.sh -f "$FILES" -t $TIME -r $CORE &
fi

if [ $COUNT_NW -gt 0 ]; then
    ./iperf-bench.sh -m c -c $COUNT_NW -a 192.168.50.7 -t $TIME -r $CORE &
fi

