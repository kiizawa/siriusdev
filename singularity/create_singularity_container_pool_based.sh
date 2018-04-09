#!/bin/bash

set -ex

SINGULARITY_IMAGE=/dev/shm/siriusdev.img
HOST=`hostname`
CEPH_NET=10.10.1.0/16
CEPH_CONF_DIR=/tmp/share

DUMMY_HOME_DIR_SSD=/tmp/kiizawa.ssd
if [ ! -e $DUMMY_HOME_DIR_SSD ]
then
    mkdir $DUMMY_HOME_DIR_SSD
fi

DUMMY_HOME_DIR_HDD=/tmp/kiizawa.hdd
if [ ! -e $DUMMY_HOME_DIR_HDD ]
then
    mkdir $DUMMY_HOME_DIR_HDD
fi

CEPH_DIR_SSD=/tmp/ceph.ssd
if [ ! -e $CEPH_DIR_SSD ]
then
    mkdir $CEPH_DIR_SSD
fi

CEPH_DIR_HDD=/tmp/ceph.hdd
if [ ! -e $CEPH_DIR_HDD ]
then
    mkdir $CEPH_DIR_HDD
fi

CLIENTS=""
SERVERS="node-1"

function start() {

    singularity instance.start \
	--writable \
        -H $DUMMY_HOME_DIR \
	-B $CEPH_CONF_DIR:/ceph_conf \
	-B $CEPH_DIR:/ceph \
	$SINGULARITY_IMAGE $INSTANCE

    SINGULARITYENV_RUN_MON=$RUN_MON \
    SINGULARITYENV_RUN_OSD=$RUN_OSD \
    SINGULARITYENV_CEPH_CONF_DIR=/ceph_conf \
    SINGULARITYENV_CEPH_DIR=/ceph \
    SINGULARITYENV_POOL_SIZE=1 \
    SINGULARITYENV_PG_NUM=$PG_NUM \
    SINGULARITYENV_OP_THREADS=32 \
    SINGULARITYENV_OSD_TYPE=$OSD_TYPE \
    SINGULARITYENV_POOL=$POOL \
    SINGULARITYENV_CEPH_PUBLIC_NETWORK=$CEPH_NET \
    SINGULARITYENV_HOSTNAME=$HOSTNAME \
    SINGULARITYENV_BS_FAST_CREATE=false \
    SINGULARITYENV_BS_SLOW_BD=$BS_SLOW_BD \
        singularity run --writable instance://$INSTANCE /root/start_ceph.sh

    sleep 3

    singularity exec instance://$INSTANCE \
	bash -c 'LD_LIBRARY_PATH=/usr/local/lib ceph -s -c /ceph_conf/ceph.conf'
}

power2() { echo "x=l($1)/l(2); scale=0; 2^((x+0.5)/1)" | bc -l; }

OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

# clients

#set +e
#R=`echo $CLIENTS | grep $HOST`
#set -e

#if [ -n "$R" ]
#then
#    HOST_NAME=$HOST"-docker"
#    HOST_ADDR=`get_client_ip_addr $HOST`
#    RUN_MON=0
#    RUN_OSD=0
#    start
#    exit
#fi

# servers

set +e
R=`echo $SERVERS | grep $HOST`
set -e

FIRST_SERVER=`echo $SERVERS | cut -d ' ' -f 1`

if [ -n "$R" ]
then

    HOSTNAME=$HOST"-singularity-ssd"
    if [ $HOST = $FIRST_SERVER ]
    then
	RUN_MON=1
	RUN_OSD=1
    else
	RUN_MON=0
	RUN_OSD=1
    fi
    POOL="cache_pool"
    OSD_TYPE="bluestore"
    DUMMY_HOME_DIR=$DUMMY_HOME_DIR_SSD
    CEPH_DIR=$CEPH_DIR_SSD
    BS_SLOW_BD=/dev/sdc
    INSTANCE=ssd
    start

    sleep 5

    HOSTNAME=$HOST"-singularity-hdd"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DUMMY_HOME_DIR=$DUMMY_HOME_DIR_HDD
    CEPH_DIR=$CEPH_DIR_HDD
    BS_SLOW_BD=/dev/sdb
    INSTANCE=hdd
    start
fi
