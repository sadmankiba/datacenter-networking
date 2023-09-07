##############################################
# Create and configure a SPDK NVMe-oF target # 
##############################################

# Steps
# 1. Create a transport. Not required after starting nvmf_tgt again.
# 2. Create a subsystem. Not required after starting nvmf_tgt again.
# 3. Create and add a malloc block device. If a block device name already exists, it cannot be created again. 
# A block device is owned by a namespace. 
# 4. Add a listener

SPDK_PATH=/home/sadman/spdk
TASK="tr" # tr | subsys | bdev | lstnr
ADDR=192.168.50.7
SIZE=$((1 * 1024)) # 1 GB
PORT_START=4420
NSID_START=1
COUNT=1
TR=tcp  # rdma | tcp

cd $SPDK_PATH

while [[ -n $1 ]]; do
    case $1 in
        -c | --count) shift
            COUNT=$1
            ;;
	    -a | --addr) shift
	        ADDR=$1
	        ;;
        -t | --transport) shift
            TR=$1
            ;;
        -s | --size) shift
            SIZE=$1
            ;;
        -h | --help) shift
            echo "Usage: ./spdk-nvmf-tgt.sh -a <ADDR> -b <BLK_NAME> -s <SIZE> -n <NSID>"
            echo "-c | --count  Number of bdevs to export. Default 1"
            echo "-a | --addr  Target address. Default 192.168.50.7"
            echo "-t | --transport  Transport. Default tcp"
            echo "-s | --size  Block device size in MB. Default 1 GB"
            echo "-n | --nsid  Namespace ID. Default 1"
            exit 0
            ;;
    esac
    shift
done

# Run nvmf_tgt. 
# Can be run run separately.
# To remove export, kill nvmf_tgt process
./build/bin/nvmf_tgt -e 0xFFFF -m 0x80 &
sleep 5

# Create a transport
./scripts/rpc.py nvmf_create_transport -t tcp -u 8192 -z

VOL_NAME=vol

# Adding bdev with malloc backend
# ./scripts/rpc.py bdev_malloc_create -b $VOL_NAME $SIZE $BLK_SIZE

# Adding bdev with nvme backend
./scripts/rpc.py bdev_nvme_attach_controller -b $VOL_NAME -t PCIe -a 0000:22:00.0 
sleep 5

function single_vol() {
    NQN=nqn.2023-04.spdk:n$1
    # Create a subsystem
    ./scripts/rpc.py nvmf_create_subsystem $NQN -a -s SPDK1 -r
    
    BLK_SIZE=512
    NSID=$(($NSID_START + $1))
    PORT=$(($PORT_START + $1))

    ./scripts/rpc.py nvmf_subsystem_add_ns -n $NSID $NQN $VOL_NAME

    # Add a listener
    ./scripts/rpc.py nvmf_subsystem_add_listener $NQN -t $TR -a $ADDR -s $PORT
}

for ((i=0; i<$COUNT; i++)); do
    single_vol $i &
done
