#!/bin/bash

set -ex

DOCKER_IMAGE=kiizawa/siriusdev:memstore_journal_monip_xattr
CLIENTS="node-0"
SERVERS="node-1 node-2"

declare -A IP_ADDRS
IP_ADDRS=(
["node-0"]="192.168.0.10"
["node-1"]="192.168.0.11"
["node-2"]="192.168.0.12"
["node-3"]="192.168.0.13"
["node-4"]="192.168.0.14"
["node-5"]="192.168.0.15"
["node-6"]="192.168.0.16"
["node-7"]="192.168.0.17"
["node-8"]="192.168.0.18"
["node-9"]="192.168.0.19"
["node-10"]="192.168.0.20"
["node-11"]="192.168.0.21"
["node-12"]="192.168.0.22"
["node-13"]="192.168.0.23"
["node-14"]="192.168.0.24"
)

function start() {
    docker run -it -d --privileged  \
	--name $HOST_NAME \
	--net cephnet \
	--hostname $HOST_NAME \
	--ip $HOST_ADDR \
	-v /dev/shm:/dev/shm \
	-v /tmp/share:/share \
	-e CEPH_CONF_DIR=/share \
	-e RUN_MON=$RUN_MON \
	-e RUN_OSD=$RUN_OSD \
	-e CEPH_CONF_DIR=$CEPH_CONF_DIR \
	-e CEPH_DIR=$CEPH_DIR \
	-e LOG_DIR=$LOG_DIR \
	-e OSD_JOURNAL=$OSD_JOURNAL \
	-e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
	-e POOL=$POOL \
	-e CEPH_PUBLIC_NETWORK=$CEPH_NET \
	$CONFIG_OPTS \
	$DOCKER_IMAGE
}

power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

CEPH_NET=192.168.0.0/16
HOST=`hostname`
HOST_NAME=$HOST"-docker"
HOST_ADDR=${IP_ADDRS[$HOST]}

LOG_DIR=/dev/shm/$HOST_NAME
if [ -e "$LOG_DIR" ]
then
    sudo rm -rf $LOG_DIR/*
else
    mkdir $LOG_DIR
fi

OSD_JOURNAL=/tmp/journal

CEPH_CONF_DIR=/share

CEPH_DIR=/dev/shm/ceph
if [ -e "$CEPH_DIR" ]
then
    sudo rm -rf $CEPH_DIR/*
else
    mkdir $CEPH_DIR
fi

OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32 -e BS_CACHE_SIZE=0"

# clients

set +e
R=`echo $CLIENTS | grep $HOST`
set -e

if [ -n "$R" ]
then
    RUN_MON=0
    RUN_OSD=0
    start
    exit
fi

#servers

set +e
R=`echo $SERVERS | grep $HOST`
set -e

FIRST_SERVER=`echo $SERVERS | cut -d ' ' -f 1`

if [ -n "$R" ]
then
    if [ $HOST = $FIRST_SERVER ]
    then
	RUN_MON=1
	RUN_OSD=1
    else
	RUN_MON=0
	RUN_OSD=1
    fi
    OSD_TYPE="filestore"
    DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
    POOL="storage_pool"
    start
fi
