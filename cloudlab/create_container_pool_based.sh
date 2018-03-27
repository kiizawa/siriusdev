#!/bin/bash

set -ex

function start() {
    rm -rf /dev/shm/$HOST_NAME; mkdir /dev/shm/$HOST_NAME
    docker run -it -d --privileged  \
	--name $HOST_NAME \
	--net cephnet \
	--hostname $HOST_NAME \
	--ip $HOST_ADDR \
	-v /dev/shm/$HOST_NAME:/dev/shm \
	-v /tmp/share:/share \
	-e CEPH_CONF_DIR=/share \
	-e RUN_MON=$RUN_MON \
	-e RUN_OSD=$RUN_OSD \
	-e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
	-e POOL=$POOL \
	-e CEPH_PUBLIC_NETWORK=$CEPH_NET \
	$CONFIG_OPTS \
	$DOCKER_IMAGE
}

DOCKER_IMAGE=kiizawa/siriusdev:ssh_pg_log
CEPH_NET=192.168.0.0/16

CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=128 -e OP_THREADS=32 -e BS_CACHE_SIZE=0"

# clients

if [ `hostname` = "node-0" ]
then
    HOST_NAME=`hostname`"-docker"
    HOST_ADDR="192.168.0.10"
    RUN_MON=0
    RUN_OSD=0
    start
fi

if [ `hostname` = "node-1" ]
then
    HOST_NAME=`hostname`"-docker"
    HOST_ADDR="192.168.0.11"
    RUN_MON=0
    RUN_OSD=0
    start
fi

# servers

declare -A IP_ADDRS
IP_ADDRS=(
["node-2-docker-ssd"]="192.168.0.12"
["node-2-docker-hdd"]="192.168.0.13"
["node-3-docker-ssd"]="192.168.0.14"
["node-3-docker-hdd"]="192.168.0.15"
["node-4-docker-ssd"]="192.168.0.16"
["node-4-docker-hdd"]="192.168.0.17"
["node-5-docker-ssd"]="192.168.0.18"
["node-5-docker-hdd"]="192.168.0.19"
)

if [ `hostname` = "node-2" ]
then
    HOST_NAME=`hostname`"-docker-ssd"
    HOST_ADDR=${IP_ADDRS[$HOST_NAME]}
    RUN_MON=1
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=`hostname`"-docker-hdd"
    HOST_ADDR=${IP_ADDRS[$HOST_NAME]}
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi

HOSTS="node-3 node-4 node-5"
for HOST in $HOSTS
do
if [ `hostname` = $HOST ]
then
    HOST_NAME=`hostname`"-docker-ssd"
    HOST_ADDR=${IP_ADDRS[$HOST_NAME]}
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=`hostname`"-docker-hdd"
    HOST_ADDR=${IP_ADDRS[$HOST_NAME]}
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi
done
