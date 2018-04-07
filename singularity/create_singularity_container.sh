#!/bin/bash

set -ex

SINGULARITY_IMAGE=/dev/shm/siriusdev.img
HOST_ADDR=`hostname -i`
CEPH_NET=`ifconfig | grep $HOST_ADDR  | cut -d ':' -f 3 | cut -d ' ' -f 1`

HOME_DIR=/home/dummy
if [ ! -e $HOME_DIR ]
then
    mkdir $HOME_DIR
fi

CLIENTS="node-0"
SERVERS="node-1"

function start() {

    #rm -rf /dev/shm/$HOST_NAME; mkdir /dev/shm/$HOST_NAME

    SINGULARITYENV_POOL_SIZE=1 \
    SINGULARITYENV_PG_NUM=$PG_NUM \
    SINGULARITYENV_OP_THREADS=32 \
    SINGULARITYENV_BS_CACHE_SIZE=0 \
    SINGULARITYENV_CEPH_CONF_DIR=/tmp/share \
    SINGULARITYENV_RUN_MON=$RUN_MON \
    SINGULARITYENV_RUN_OSD=$RUN_OSD \
    SINGULARITYENV_LOG_DIR=$LOG_DIR \
    SINGULARITYENV_OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
    SINGULARITYENV_POOL=$POOL \
    SINGULARITYENV_CEPH_PUBLIC_NETWORK=$CEPH_NET \
    singularity shell --writable -H $HOME_DIR $IMAGE
}

function power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

HOST=`hostname`
#HOST_NAME=$HOST"-docker"
#HOST_ADDR=${IP_ADDRS[$HOST]}
LOG_DIR=/dev/shm

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
    OSD_TYPE="bluestore"
    #DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
    POOL="storage_pool"
    start
fi
