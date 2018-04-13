#!/bin/bash

set -ex

# settings

DOCKER_IMAGE=kiizawa/siriusdev:memstore_journal_xattr_monip
CEPH_CONF_DIR=/tmp/share

CEPH_DIR_SSD=/dev/shm/ceph
NEED_XATTR_SSD=1

CEPH_DIR_HDD=/tmp/share/ceph
NEED_XATTR_HDD=1

CEPH_NET=192.168.0.0/16

OSD_NUM_PER_POOL=1

source /tmp/role_decider.sh

function start() {

    if [ $POOL = "cache_pool" ]
    then
	CEPH_DIR_LOGICAL=$CEPH_DIR_SSD
	NEED_XATTR=$NEED_XATTR_SSD
    fi

    if [ $POOL = "storage_pool" ]
    then
	CEPH_DIR_LOGICAL=$CEPH_DIR_HDD
	NEED_XATTR=$NEED_XATTR_HDD
    fi

    CEPH_DIR_PHYSICAL=$CEPH_DIR_LOGICAL.$HOST_NAME
    if [ ! -e "$CEPH_DIR_PHYSICAL" ]
    then
	mkdir $CEPH_DIR_PHYSICAL
    else
	sudo rm -rf $CEPH_DIR_PHYSICAL/*
    fi

    docker run -it -d --privileged  \
	--name $HOST_NAME \
	--net cephnet \
	--hostname $HOST_NAME \
	--ip $HOST_ADDR \
	-v $CEPH_CONF_DIR:$CEPH_CONF_DIR \
	-v $CEPH_DIR_PHYSICAL:$CEPH_DIR_LOGICAL \
	-e CEPH_CONF_DIR=$CEPH_CONF_DIR \
	-e CEPH_DIR=$CEPH_DIR_LOGICAL \
	-e RUN_MON=$RUN_MON \
	-e RUN_OSD=$RUN_OSD \
	-e NEED_XATTR=$NEED_XATTR \
	-e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
	-e POOL=$POOL \
	-e CEPH_PUBLIC_NETWORK=$CEPH_NET \
	$CONFIG_OPTS \
	$DOCKER_IMAGE
}

power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

HOST=`hostname`

#OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32"

# servers

HOST_NAME=$HOST"-docker"
HOST_ADDR=`get_ip_addr`

RUN_MON=`is_mon`
RUN_OSD=1
POOL=`get_pool`
OSD_TYPE="filestore"

start
