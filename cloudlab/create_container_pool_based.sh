#!/bin/bash

set -ex

function start() {
    docker run -it -d --privileged  \
	--name $HOST_NAME \
	--net cephnet \
	--hostname $HOST_NAME \
	--ip $HOST_ADDR \
	--shm-size 4096MB \
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

DOCKER_IMAGE=kiizawa/siriusdev
CEPH_NET=192.168.0.0/16

CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=128 -e OP_THREADS=32 -e BS_CACHE_SIZE=0"

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
    HOST_NAME=`hostname`"-docker-ssd"
    HOST_ADDR="192.168.0.11"
    RUN_MON=1
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=`hostname`"-docker-hdd"
    HOST_ADDR="192.168.0.12"
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi

if [ `hostname` = "node-2" ]
then
    HOST_NAME=`hostname`"-docker-ssd"
    HOST_ADDR="192.168.0.13"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=`hostname`"-docker-hdd"
    HOST_ADDR="192.168.0.14"
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi

if [ `hostname` = "node-3" ]
then
    HOST_NAME=`hostname`"-docker-ssd"
    HOST_ADDR="192.168.0.15"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=`hostname`"-docker-hdd"
    HOST_ADDR="192.168.0.16"
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi

if [ `hostname` = "node-4" ]
then
    HOST_NAME=`hostname`"-docker-ssd"
    HOST_ADDR="192.168.0.17"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc"
    start

    sleep 5

    HOST_NAME=`hostname`"-docker-hdd"
    HOST_ADDR="192.168.0.18"
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb"
    start
fi
