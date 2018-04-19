#!/bin/sh

set -ex

source $MODULESHOME/init/bash
module load singularity

# settings

SINGULARITY_IMAGE=$PROJWORK/csc143/ceph/ceph_on_titan_v1.img
CEPH_CONF_DIR=$PROJWORK/csc143/$USER/ceph/conf

CEPH_DIR_SSD=/dev/shm/$USER/ceph/data
NEED_XATTR_SSD=1

CEPH_DIR_HDD=$PROJWORK/csc143/$USER/ceph/data
NEED_XATTR_HDD=0

CEPH_NET=10.128.0.0/14

OSD_NUM_PER_POOL=1

CEPH_SCRIPTS_DIR=$PROJWORK/csc143/$USER/ceph/scripts
source $CEPH_SCRIPTS_DIR/role_decider.sh

function start() {

    #DUMMY_HOME_DIR=/tmp/kiizawa
    #if [ ! -e $DUMMY_HOME_DIR ]
    #then
	#mkdir $DUMMY_HOME_DIR
    #fi

    if [ $POOL = "cache_pool" ]
    then
	CEPH_DIR=$CEPH_DIR_SSD
	NEED_XATTR=$NEED_XATTR_SSD
    fi

    if [ $POOL = "storage_pool" ]
    then
	CEPH_DIR=$CEPH_DIR_HDD
	NEED_XATTR=$NEED_XATTR_HDD
    fi

    CEPH_DIR=$CEPH_DIR.$CONTAINER_NAME.$$
    if [ ! -e "$CEPH_DIR" ]
    then
	mkdir -p $CEPH_DIR
    else
	rm -rf $CEPH_DIR/*
    fi

    if [ -n "$RUN_MON" -a $RUN_MON = 1 ]
    then
	CEPH_NET_PREFIX=`echo $CEPH_NET | cut -d . -f 1-2`
	MON_ADDR=`ip addr show | grep "inet " | sed -e 's/^[ ]*//g' | cut -d ' ' -f 2 | cut -d '/' -f 1 | grep $CEPH_NET_PREFIX | cut -d ' ' -f 1`
    fi

    #singularity instance.start \
	#--writable \
        #-H $DUMMY_HOME_DIR \
	#-B $CEPH_CONF_DIR:/ceph_conf \
	#-B $CEPH_DIR_PHYSICAL:/ceph \
	#$SINGULARITY_IMAGE siriusdev

    #singularity instance.start \
    #$SINGULARITY_IMAGE siriusdev
    #PID=`singularity instance.list | grep siriusdev | sed -e 's/\s\+/ /g' | cut -d ' ' -f 2`
    #ls /proc/$PID/ns

    SINGULARITYENV_NEED_XATTR=$NEED_XATTR \
    SINGULARITYENV_CEPH_CONF_DIR=$CEPH_CONF_DIR \
    SINGULARITYENV_CEPH_DIR=$CEPH_DIR \
    SINGULARITYENV_RUN_MON=$RUN_MON \
    SINGULARITYENV_RUN_OSD=$RUN_OSD \
    SINGULARITYENV_OSD_TYPE=$OSD_TYPE \
    SINGULARITYENV_POOL=$POOL \
    SINGULARITYENV_CEPH_PUBLIC_NETWORK=$CEPH_NET \
    SINGULARITYENV_MON_ADDR=$MON_ADDR \
    SINGULARITYENV_POOL_SIZE=1 \
    SINGULARITYENV_PG_NUM=$PG_NUM \
    SINGULARITYENV_OP_THREADS=32 \
    SINGULARITYENV_EXIT_AFTER_START=0 \
    singularity exec $SINGULARITY_IMAGE /root/start_ceph.sh

    #singularity run --writable instance://siriusdev /root/start_ceph.sh
    #singularity help run
    #singularity exec instance://siriusdev /bin/date
}

power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

HOST=`hostname`

#OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

#CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32"

# servers

CONTAINER_NAME=$HOST"-singularity"
#HOST_ADDR=`get_ip_addr`

RUN_MON=`is_mon`
RUN_OSD=1
POOL=`get_pool`
OSD_TYPE="filestore"

start
