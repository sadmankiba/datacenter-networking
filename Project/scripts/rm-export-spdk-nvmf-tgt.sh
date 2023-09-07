
SPDK_PATH=/home/sadman/spdk
NQN_EXT=cnode1
BLK_NAME=Malloc0
ADDR=192.168.50.7
NSID=1

cd $SPDK_PATH

# Delete listener
./scripts/rpc.py nvmf_subsystem_remove_listener nqn.2023-04.io.spdk:$NQN_EXT -t tcp -a $ADDR -s 4420

# Delete block device
./scripts/rpc.py bdev_malloc_delete $BLK_NAME

# Delete namespace
./scripts/rpc.py nvmf_subsystem_remove_ns nqn.2023-04.io.spdk:$NQN_EXT $NSID

# Delete subsystem
./scripts/rpc.py nvmf_delete_subsystem nqn.2023-04.io.spdk:$NQN_EXT