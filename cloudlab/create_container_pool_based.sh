#!/bin/bash

set -ex

DOCKER_IMAGE=kiizawa/siriusdev:ssh_pg_log
CLIENTS="node-0 node-1"
SERVERS="node-5 node-6 node-7 node-8"

declare -A CLIENT_IP_ADDRS
CLIENT_IP_ADDRS=(
["node-0"]="192.168.0.10"
["node-1"]="192.168.0.11"
["node-2"]="192.168.0.12"
["node-3"]="192.168.0.13"
["node-4"]="192.168.0.14"
)

declare -A SERVER_IP_ADDRS
SERVER_IP_ADDRS=(
["node-5-docker-ssd"]="192.168.0.15"
["node-5-docker-hdd"]="192.168.0.16"
["node-6-docker-ssd"]="192.168.0.17"
["node-6-docker-hdd"]="192.168.0.18"
["node-7-docker-ssd"]="192.168.0.19"
["node-7-docker-hdd"]="192.168.0.20"
["node-8-docker-ssd"]="192.168.0.21"
["node-8-docker-hdd"]="192.168.0.22"
["node-9-docker-ssd"]="192.168.0.23"
["node-9-docker-hdd"]="192.168.0.24"
["node-10-docker-ssd"]="192.168.0.25"
["node-10-docker-hdd"]="192.168.0.26"
["node-11-docker-ssd"]="192.168.0.27"
["node-11-docker-hdd"]="192.168.0.28"
["node-12-docker-ssd"]="192.168.0.29"
["node-12-docker-hdd"]="192.168.0.30"
["node-13-docker-ssd"]="192.168.0.31"
["node-13-docker-hdd"]="192.168.0.32"
["node-14-docker-ssd"]="192.168.0.33"
["node-14-docker-hdd"]="192.168.0.34"
)

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

CEPH_NET=192.168.0.0/16
HOST=`hostname`

OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
if [ $OSD_NUM_PER_POOL -lt 5 ]
then
    PG_NUM=128
elif [ $OSD_NUM_PER_POOL -lt 10 ]
then
    PG_NUM=512
elif [ $OSD_NUM_PER_POOL -lt 50 ]
then
    PG_NUM=1024
fi

CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32 -e BS_CACHE_SIZE=0"

# clients

set +e
R=`echo $CLIENTS | grep $HOST`
set -e

if [ -n "$R" ]
then
    HOST_NAME=$HOST"-docker"
    HOST_ADDR=${CLIENT_IP_ADDRS[$HOST]}
    RUN_MON=0
    RUN_OSD=0
    start
fi

# servers

set +e
R=`echo $SERVERS | grep $HOST`
set -e

FIRST_SERVER=`echo $SERVERS | cut -d ' ' -f 1`

if [ -n "$R" ]
then
    HOST_NAME=$HOST"-docker-ssd"
    HOST_ADDR=${SERVER_IP_ADDRS[$HOST_NAME]}
    if [ $HOST = $FIRST_SERVER ]
    then
	RUN_MON=1
	RUN_OSD=1
    else
	RUN_MON=0
	RUN_OSD=1
    fi
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=$HOST"-docker-hdd"
    HOST_ADDR=${SERVER_IP_ADDRS[$HOST_NAME]}
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi
