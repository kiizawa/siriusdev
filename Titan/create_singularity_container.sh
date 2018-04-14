#!/bin/bash

set -ex

# settings

SINGULARITY_IMAGE=/dev/shm/siriusdev.img

CEPH_CONF_DIR=/tmp/share

CEPH_DIR_SSD=/dev/shm/ceph
NEED_XATTR_SSD=1

CEPH_DIR_HDD=/tmp/share/ceph
NEED_XATTR_HDD=1

CEPH_NET=128.104.222.0/23

OSD_NUM_PER_POOL=1

source /tmp/role_decider.sh

function start() {

    DUMMY_HOME_DIR=/tmp/kiizawa
    if [ ! -e $DUMMY_HOME_DIR ]
    then
	sudo mkdir $DUMMY_HOME_DIR
    fi

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

    CEPH_DIR_PHYSICAL=$CEPH_DIR_LOGICAL.$CONTAINER_NAME
    if [ ! -e "$CEPH_DIR_PHYSICAL" ]
    then
	mkdir $CEPH_DIR_PHYSICAL
    else
	sudo rm -rf $CEPH_DIR_PHYSICAL/*
    fi

    if [ -n "$RUN_MON" -a $RUN_MON = 1 ]
    then
	CEPH_NET_PREFIX=`echo $CEPH_NET | cut -d . -f 1-2`
	MON_ADDR=`ip addr show | grep "inet " | sed -e 's/^[ ]*//g' | cut -d ' ' -f 2 | cut -d '/' -f 1 | grep $CEPH_NET_PREFIX`
    fi

    sudo singularity instance.start \
	--writable \
        -H $DUMMY_HOME_DIR \
	-B $CEPH_CONF_DIR:/ceph_conf \
	-B $CEPH_DIR_PHYSICAL:/ceph \
	$SINGULARITY_IMAGE siriusdev

    sudo \
    SINGULARITYENV_NEED_XATTR=$NEED_XATTR \
    SINGULARITYENV_CEPH_CONF_DIR=/ceph_conf \
    SINGULARITYENV_CEPH_DIR=/ceph \
    SINGULARITYENV_RUN_MON=$RUN_MON \
    SINGULARITYENV_RUN_OSD=$RUN_OSD \
    SINGULARITYENV_OSD_TYPE=$OSD_TYPE \
    SINGULARITYENV_POOL=$POOL \
    SINGULARITYENV_CEPH_PUBLIC_NETWORK=$CEPH_NET \
    SINGULARITYENV_MON_ADDR=$MON_ADDR \
    SINGULARITYENV_POOL_SIZE=1 \
    SINGULARITYENV_PG_NUM=$PG_NUM \
    SINGULARITYENV_OP_THREADS=32 \
        singularity run --writable instance://siriusdev /root/start_ceph.sh

    singularity exec instance://siriusdev \
	bash -c 'LD_LIBRARY_PATH=/usr/local/lib ceph -s -c /ceph_conf/ceph.conf'
}

power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

HOST=`hostname`

#OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

#CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32"

# servers

CONTAINER_NAME=$HOST"-singularity"
HOST_ADDR=`get_ip_addr`

RUN_MON=`is_mon`
RUN_OSD=1
POOL=`get_pool`
OSD_TYPE="filestore"

start
