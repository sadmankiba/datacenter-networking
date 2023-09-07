#!/bin/bash


trap 'exit' ERR

[ -z ${SDE} ] && echo "Environment variable SDE not set" && exit 1
[ -z ${SDE_INSTALL} ] && echo "Environment variable SDE_INSTALL not set" && exit 1

echo "Using SDE ${SDE}"
echo "Using SDE_INSTALL ${SDE_INSTALL}"

# P4 program name
P4_NAME="switch"
# Number of subdevices per device (Package size)
NUM_SUBDEVICES=1
# json file specifying model port to veth mapping info
PORTINFO=None
MAX_PORTS=17
NUM_PIPES=4
CPUPORT=64
CPUVETH=251

NO_VETH=false
HELP=false
SETUP=""
CLEANUP=""
ARCH="tofino"
CHIP_FAMILY="tofino"
TARGET="asic-model"

SOCKET_RECV_SIZE="10240"
SKIP_STATUS_SRV=false
TEST_PARAMS=""
DRV_TEST_INFO=""
DRV_TEST_SEED=""
PORTMODE=""
GEN_XML_OUTPUT=0
DB_PREFIX=""
P4INFO_PATH=""
P4_NAME_OPTION=""
PMM=""
NUM_SHARDS=""
SHARD_ID=""

THRIFT_SERVER='localhost'
GRPC_SERVER='localhost'
STATUS_SERVER='localhost'

if [ $NO_VETH = true ]; then
  CPUPORT=None
  CPUVETH=None
else
  # Setup veth interfaces
  echo "Setting up veth interfaces"
  sudo env "PATH=$PATH" $SDE_INSTALL/bin/veth_setup.sh
fi

# TEST_DIR=`find $SDE/pkgsrc -type d -path "*switch*/ptf/api"`
TEST_DIR=$SDE/port-setup

P4_NAME_OPTION="--p4-name=$P4_NAME"

if [ ! -d "$TEST_DIR" ]; then
  echo "Test directory \"$TEST_DIR\" does not exist"
  exit 1
fi

if [[ ! -r $PORTINFO ]]; then
  PORTINFO_FILE=$TEST_DIR/ports.json
  if [[ -r $PORTINFO_FILE ]]; then
    PORTINFO=$PORTINFO_FILE
  fi
fi

if [[ $PORTINFO != *None ]]; then
  CPUPORT=None
  CPUVETH=None
fi

export PATH=$SDE_INSTALL/bin:$PATH
echo "Using TEST_DIR ${TEST_DIR}"
if [[ -r $PORTINFO ]]; then
  echo "Using Port Info File $PORTINFO"
fi
echo "Using PATH ${PATH}"
echo "Arch is $ARCH"
echo "Target is $TARGET"
PYTHON_LIB_DIR=$(python3 -c "from distutils import sysconfig; print(sysconfig.get_python_lib(prefix='', standard_lib=True, plat_specific=True))")

export PYTHONPATH=$SDE_INSTALL/$PYTHON_LIB_DIR/site-packages/tofino/bfrt_grpc:$PYTHONPATH
export PYTHONPATH=$SDE_INSTALL/$PYTHON_LIB_DIR/site-packages:$PYTHONPATH
export PYTHONPATH=$SDE_INSTALL/$PYTHON_LIB_DIR/site-packages/tofino:$PYTHONPATH
export PYTHONPATH=$SDE_INSTALL/$PYTHON_LIB_DIR/site-packages/${ARCH}pd:$PYTHONPATH
export PYTHONPATH=$SDE_INSTALL/$PYTHON_LIB_DIR/site-packages/p4testutils:$PYTHONPATH

if [ "${PKTPY,,}" = "false" ]; then
  echo "Using scapy."
else
  echo "Using bf_pktpy."
  PMM="-pmm bf_pktpy.ptf.packet_pktpy"
fi

# Check in with bf_switchd's status server to make sure it is ready
STS_PORT_STR="--port 7777"
if [ "$STS_PORT" != "" ]; then
  STS_PORT_STR="--port $STS_PORT"
fi
STS_HOST_STR="--host $STATUS_SERVER"
if [ $SKIP_STATUS_SRV = false ]; then
  python3 $SDE_INSTALL/$PYTHON_LIB_DIR/site-packages/p4testutils/bf_switchd_dev_status.py \
    $STS_HOST_STR $STS_PORT_STR
fi

# P4Runtime PTF tests require the P4Info path to be provided as a PTF KV
# parameter. If the script was invoked with --p4info, we use the provided
# value. Otherwise we look for the P4Info file in the install directory.
if [ -z "$P4INFO_PATH" ]; then
    if [ -n "$P4_NAME" ]; then
        p4info=$SDE_INSTALL/share/${ARCH}pd/$P4_NAME/p4info.pb.txt
        if [ -f $p4info ]; then
            P4INFO_PATH=$p4info
        fi
    fi
fi
if [ "$P4INFO_PATH" != "" ]; then
    if [ "$TEST_PARAMS" != "" ]; then
        TEST_PARAMS="$TEST_PARAMS;p4info='$P4INFO_PATH'"
    else
        TEST_PARAMS="p4info='$P4INFO_PATH'"
    fi
fi

TEST_PARAMS_STR=""
if [ "$TEST_PARAMS" != "" ]; then
    TEST_PARAMS_STR="--test-params $TEST_PARAMS"
fi

echo "Configuring switch ports"

#Run PTF tests
sudo env "PATH=$PATH" "PYTHONPATH=$PYTHONPATH" "GEN_XML_OUTPUT=$GEN_XML_OUTPUT" "PKTPY=$PKTPY" python3 \
    $SDE_INSTALL/$PYTHON_LIB_DIR/site-packages/p4testutils/run_ptf_tests.py \
    $PMM \
    $NUM_SHARDS $SHARD_ID \
    --arch $CHIP_FAMILY \
    --target $TARGET \
    --test-dir $TEST_DIR \
    --port-info $PORTINFO \
    $PORTMODE \
    $P4_NAME_OPTION \
    --thrift-server $THRIFT_SERVER \
    --grpc-server $GRPC_SERVER \
    --cpu-port $CPUPORT \
    --cpu-veth $CPUVETH \
    --max-ports $MAX_PORTS \
    --num-pipes $NUM_PIPES \
    --socket-recv-size $SOCKET_RECV_SIZE \
    $DRV_TEST_INFO $DRV_TEST_SEED \
    $TEST_SUITE $SETUP $CLEANUP $TEST_PARAMS_STR $DB_PREFIX $@ &> configure-ports.log &

res=$?
if [[ $res == 0 ]]; then
    exit $res
else
    exit 1
fi
