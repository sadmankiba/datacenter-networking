#!/bin/bash

TR_TYPE=tcp     # tcp or rdma
ADDR=192.168.50.7 # target address
CMD=cn # cn or dcn
PORT_START=4420
DEV=/dev/nvme2n
COUNT=1

while [[ -n $1 ]]; do
    case $1 in
        -t | --tr)   shift
            TR_TYPE=$1
            ;;
        -a | --addr) shift
            ADDR=$1
            ;;
        -m | --cmd) shift
            CMD=$1
            ;;
        -p | --port) shift
            PORT_START=$1
            ;;
        -c | --count) shift
            COUNT=$1
            ;;
        -h | --help) shift
            echo "Usage: ./nvme-con.sh [-t tcp|rdma] [-a <addr>] [-m dcv|cn|dcn] [-p port] [-c count] [-h]"
            echo "Options:"
            echo "-t | --tr:   Transport type. Valid values are: tcp, rdma. Default: tcp"
            echo "-a | --addr:   Target address. Default: 192.168.50.7"
            echo "-m | --cmd:   Command to run. Valid values are: dcv, cn, dcn. Default: cn"
            echo "-p | --port:   Port number start. Default: 4420"
            echo "-c | --count:   Number of ports to connect/disconnect. Default: 1"
            exit 0
            ;;
    esac
    shift
done

function single_port() {
    PORT=$(($PORT_START + $1))
    VOL=$(($1 + 1))
    if [ $CMD == "dcn" ]; then
        nvme disconnect ${DEV}${VOL} -n nvme-test
    elif [ $CMD == "dcv" ]; then
        # Also connects if previously connected
        nvme discover -t $TR_TYPE -a $ADDR -s $PORT  
    else 
        nvme discover -t $TR_TYPE -a $ADDR -s $PORT
        nvme connect -t $TR_TYPE -n nvme-test -a $ADDR -s $PORT
    fi
}

set -x
for i in $(seq 0 $(($COUNT-1))); do
    single_port $i
done
