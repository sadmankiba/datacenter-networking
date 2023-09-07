# Example usage: ./rdma-test.sh -m [client | server] -l mlx5_1

while [[ -n $1 ]]; do
    case $1 in
        -m | --mode)    shift
            MODE=$1
            ;;
        -l | --link)   shift
            LINK=$1
            ;;
        -a | --addr)    shift
            ADDR=$1
            ;;
        -h | --help)    echo "Usage: ./rdma-test.sh -m [client | server] -l rdma_link"
            exit
            ;;
    esac
    shift
done

# mode server
if [ $MODE == 'server' ]; then
    ib_write_bw -d $LINK -i 1 -p 9001
else
    ib_write_bw -n 10000 -F -s 10000 -m 256 -c RC -d $LINK -i 1 -p 9001 $ADDR
fi
