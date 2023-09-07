TR_TYPE=rdma     # 'rdma' or 'tcp'

while [[ -n $1 ]]; do
    case $1 in
        -t | --tr)   shift
            TR_TYPE=$1
            ;;
    esac
    shift
done

if [ $TR_TYPE == 'rdma' ]; then
    modprobe nvme-rdma
    modprobe ib_uverbs
    modprobe rdma_rxe
    modprobe rdma_ucm
    modprobe nvmet
    modprobe nvmet-rdma
else
    modprobe nvme
    modprobe nvme-tcp
    modprobe nvmet
    modprobe nvmet-tcp
fi

