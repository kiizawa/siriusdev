#!/bin/bash

set -ex

SINGULARITY_IMAGE=/dev/shm/siriusdev.img
HOST=`hostname`
HOST_NAME=$HOST"-docker"
HOST_ADDR=`hostname -i`
CEPH_NET=`ifconfig | grep $HOST_ADDR  | cut -d ':' -f 3 | cut -d ' ' -f 1`
CEPH_CONF_DIR=/share
CEPH_DIR=/tmp/ceph

HOME_DIR=/home/dummy
if [ ! -e $HOME_DIR ]
then
    mkdir $HOME_DIR
fi

CLIENTS="node-0 node-1"
SERVERS="node-2 node-3 node-4 node-5"

function start() {
    SINGULARITYENV_RUN_MON=$RUN_MON \
    SINGULARITYENV_RUN_OSD=$RUN_OSD \
    SINGULARITYENV_CEPH_CONF_DIR=$CEPH_CONF_DIR \
    SINGULARITYENV_CEPH_DIR=$CEPH_DIR \
    SINGULARITYENV_POOL_SIZE=1 \
    SINGULARITYENV_PG_NUM=$PG_NUM \
    SINGULARITYENV_OP_THREADS=32 \
    SINGULARITYENV_OSD_TYPE=$OSD_TYPE \
    SINGULARITYENV_POOL=$POOL \
    SINGULARITYENV_CEPH_PUBLIC_NETWORK=$CEPH_NET \
    singularity shell --writable -H $HOME_DIR $SINGULARITY_IMAGE
}

power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

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

if [ -e "$CEPH_DIR" ]
then
    sudo rm -rf $CEPH_DIR/*
else
    dd if=/dev/zero of=/dev/shm/ceph_disk.img bs=1M count=20K
    sudo losetup /dev/loop0 /dev/shm/ceph_disk.img
    sudo mkfs.ext4 /dev/loop0
    mkdir $CEPH_DIR
    sudo mount /dev/loop0 $CEPH_DIR
fi

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
    POOL="storage_pool"
    start
fi
