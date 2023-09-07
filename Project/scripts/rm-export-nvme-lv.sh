# Default values
NS_START=10
PORT_START=4420
COUNT=1
DELETE_LV=0

while [[ -n $1 ]]; do
    case $1 in
        -n | --ns) shift
            NS_START=$1
            ;;
        -p | --port) shift
            PORT_START=$1
            ;;
        -c | --count) shift
            COUNT=$1
            ;;
        -d | --delete) shift
            DELETE_LV=1
            ;;
        -h | --help) shift
            # Write help message
            echo "Usage: ./rm-export-nvme-lv.sh [-n namespace] [-p port] [-c count]"
            echo "  -n|--ns <ns>  : Namespace ID start. Default: 10"
            echo "  -p|--port <port>  : Port number start. Default: 4420"
            echo "  -c|--count <count>  : Number of LVs to export. Default: 1"
            echo "  -d|--delete  : Delete the LVs after unexporting them. Default: 0"
            exit 0
            ;;
    esac
    shift
done


function rm_single_lv() {
    LV_NUM=$1

    NS=$(($NS_START + $LV_NUM))
    PORT=$(($PORT_START + $LV_NUM))

    NVMET_DIR=/sys/kernel/config/nvmet
    SUBSYS=$NVMET_DIR/subsystems/nvme-test
    NAMESPACE=$SUBSYS/namespaces/$NS
    SUBSYSL=$PORT/subsystems/nvme-test

    if [ -e $SUBSYSL ]; then
        rm $SUBSYSL
    fi

    echo 0 > $NAMESPACE/enable
    echo "LV export over NVMe at (port $PORT, NS $NS) removed"

    if [ $DELETE_LV -eq 1 ]; then
        lvremove -f /dev/ubuntu-vg/test-lv$LV_NUM
    fi
}

for i in $(seq 0 $(($COUNT-1))); do
    rm_single_lv $i
done

