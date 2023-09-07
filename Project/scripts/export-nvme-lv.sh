########################################
# Export a logical volume with NVMe-oF #
# Run this script from target          #
########################################

# Default values
EXPORT_LV=test-lv
TR_TYPE=rdma     # 'rdma' or 'tcp'
DEVICE=/dev/ubuntu-vg/$EXPORT_LV
NS_START=10
TR_ADDR=192.168.50.7 # target address
PORT_START=4420
COUNT=1

while [[ -n $1 ]]; do
    case $1 in
        -d | --device)   shift
			 DEVICE=$1
			 ;;
        -t | --tr)   shift
            TR_TYPE=$1
            ;;
        -a | --addr) shift
            TR_ADDR=$1
            #TODO: Check if an interface has this address. If no such i/f, abort.
            ;;
        -v | --lv) shift
            EXPORT_LV=$1
            DEVICE=/dev/ubuntu-vg/$EXPORT_LV
            ;;
        -n | --ns) shift
            NS_START=$1
            ;;
        -p | --port) shift
            PORT_START=$1
            ;;
        -c | --count) shift
            COUNT=$1
            ;;
        -h | --help) shift
            # Write help message
            echo "Usage: ./export-nvme-lv.sh [-t rdma|tcp] [-a addr] [-v lv] [-n namespace] [-p port] [-c count] [-d device]"
            echo "  -t|--tr <tr>  : Transport type. Valid values are: rdma, tcp. Default: rdma"
            echo "  -a|--addr <addr>  : Target address. Default: 192.168.50.7"
            echo "  -v|--lv <lv>  : Base name of logical volumes to export. Default: test-lv"
            echo "  -n|--ns <ns>  : Namespace ID start. Default: 10"
            echo "  -p|--port <port>  : Port number start. Default: 4420"
            echo "  -c|--count <count>  : Number of LVs to export. Default: 1"
            echo "  -d|--device <device>  : Device to export. e.g. /dev/nvme0n1 or /dev/ubuntu-vg/test-lv. Not required if using lv name."
            exit 0
            ;;
    esac
    shift
done

function single_lv() {
    LV_NUM=$1
    EXPORT_LV=test-lv$LV_NUM
    DEVICE=/dev/ubuntu-vg/$EXPORT_LV
    NS=$(($NS_START + $LV_NUM))

    #TODO: Kill process on port if it is running
    PORT=$(($PORT_START + $LV_NUM))

    if [ -e "$DEVICE" ]; then
        echo "To-export LV exists. Skipping creation."
    else
        echo "To-export LV does not exist. Creating..."
        # If ubuntu-vg does not exist, run 'vgcreate ubuntu-vg /dev/nvme0n1'
        lvcreate --size=50G --name=$EXPORT_LV /dev/ubuntu-vg
    fi

    NVMET_DIR=/sys/kernel/config/nvmet

    SUBSYS=$NVMET_DIR/subsystems/nvme-test
    if [ ! -d $SUBSYS ]; then
        echo "Subsystem does not exist. Creating.."
        mkdir $SUBSYS
    else 
        echo "Subsystem exists. Skipping creation.."
    fi
    cd $SUBSYS
    echo 1 > attr_allow_any_host

    NAMESPACE=$SUBSYS/namespaces/$NS
    if [ ! -d $NAMESPACE ]; then
        echo "Namespace does not exist. Creating.."
        mkdir $NAMESPACE 
    else
        echo "Namespace exists. Skipping creation.."
    fi
    cd $NAMESPACE

    #TODO: Check if the namespace is already enabled. If yes, skip.
    echo -n $DEVICE > device_path
    echo -n 1 > enable

    PORT_DIR=$NVMET_DIR/ports/$NS
    if [ ! -d $PORT_DIR ]; then
        echo "Port dir does not exist. Creating.."
        mkdir $PORT_DIR
    else
        echo "Port exists. Skipping creation.."
    fi
    cd $PORT_DIR

    SUBSYSL=$PORT_DIR/subsystems/nvme-test
    if [ -e $SUBSYSL ]; then
        rm $SUBSYSL
    fi
    echo $TR_TYPE > addr_trtype
    echo $TR_ADDR > addr_traddr
    echo $PORT > addr_trsvcid
    echo ipv4 > addr_adrfam
    ln -s $SUBSYS subsystems/nvme-test

    echo "LV exported over NVMe"
}

#TODO: Unload the other transport module if it is loaded
./load-nvme-mod.sh -t $TR_TYPE

for i in `seq 0 $(($COUNT-1))`; do
    single_lv $i
done