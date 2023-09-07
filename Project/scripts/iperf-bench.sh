#!/bin/bash

WINDOW=32M
LENGTH=64M
MODE="s"
ADDR=192.168.50.7
PORT_START=9011
COUNT=1
CORE_START=5
TIME=20

while [[ -n $1 ]]; do
    case $1 in
        -c | --count)   shift
            COUNT=$1
            ;;
        -m | --mode)   shift
			MODE=$1
			;;
        -a | --address)   shift
            ADDR=$1
            ;;
        -w | --window)   shift
            WINDOW=$1
            ;;
        -l | --length)   shift
            LENGTH=$1
            ;;
        -r | --core)   shift
            CORE_START=$1
            ;;
        -t | --time)   shift
            TIME=$1
            ;;
        -h | --help) shift
            echo "Usage: $0 [-m|--mode <mode>] [-a|--address <address>]"
            echo "  -c|--count <count>  : Number of iperf instances to run. Default: 1"
            echo "  -m|--mode <mode>  : Mode of operation. Valid values are: s, c. Default: s"
            echo "  -a|--address <address>  : Address of the server. Default: 192.168.50.7"
            echo "  -w|--window <window>  : TCP window size. Default: 32M"
            echo "  -l|--length <length>  : Buffer length. Default: 64M"
            echo "  -r|--core <core>  : Start core to run iperf on. Default: 5"
            echo "  -t|--time <time>  : Time (s) to run each test. Default: 20"
            exit 0
            ;;
    esac
    shift
done

function single_app() {
    PORT=$((PORT_START + $1))
    CORE=$((CORE_START + $1))
    if [ "$MODE" == "s" ]; then
        echo "Starting iperf server"
        taskset -c $CORE iperf -s -p $PORT -w $WINDOW -l $LENGTH > iperf-serv-$1.out &
    else
        echo "Starting iperf client"
        taskset -c $CORE iperf -c $ADDR -p $PORT -w $WINDOW -l $LENGTH -t $TIME -i 5 > iperf-clnt-$1.out &
    fi
}

for ((i=0; i<$COUNT; i++)); do
    single_app $i &
done
