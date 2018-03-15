#!/bin/bash

set -ex

NODES="node-0 node-1 node-2 node-3 node-4"

declare -A IP_ADDRS
IP_ADDRS=(
["node-0"]="192.168.0.10"
["node-1"]="192.168.0.11"
["node-2"]="192.168.0.12"
["node-3"]="192.168.0.13"
["node-4"]="192.168.0.14"
)

function start() {
    docker run -it -d --privileged  \
	--name $HOST_NAME \
	--net cephnet \
	--hostname $HOST_NAME \
	--ip $HOST_ADDR \
	-v /tmp/share:/share \
	-e CEPH_CONF_DIR=/share \
	-e RUN_MON=$RUN_MON \
	-e RUN_OSD=$RUN_OSD \
	-e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
	-e POOL=$POOL \
	-e CEPH_PUBLIC_NETWORK=$CEPH_NET \
	$DOCKER_IMAGE
}

NODE=`hostname`
HOST_NAME=$NODE"-docker"
HOST_ADDR=${IP_ADDRS[$NODE]}

if [ $NODE = "node-0" ]
then
    RUN_MON=0
    RUN_OSD=0
elif [ $NODE = "node-1" ]
then
    RUN_MON=1
    RUN_OSD=1
else
    RUN_MON=0
    RUN_OSD=1
fi

OSD_TYPE="bluestore"
DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
POOL="storage_pool"
CEPH_NET=192.168.0.0/16
DOCKER_IMAGE=kiizawa/siriusdev

start
