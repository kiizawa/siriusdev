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

source ./role_decider.sh

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

#if [ ! -e "$CEPH_CONF_DIR" ]
#then
#    mkdir $CEPH_CONF_DIR
#fi

#LOG_DIR=/dev/shm/ceph
#if [ ! -e "$LOG_DIR" ]
#then
#    mkdir $LOG_DIR
#fi

OSD_NUM_PER_POOL=`echo $SERVERS | wc -w`
PG_NUM=`expr $OSD_NUM_PER_POOL \* 100`
PG_NUM=`power2 $PG_NUM`

#CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32 -e BS_CACHE_SIZE=0"
CONFIG_OPTS="-e POOL_SIZE=1 -e PG_NUM=$PG_NUM -e OP_THREADS=32"

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

#set +e
#R=`echo $SERVERS | grep $HOST`
#set -e

#FIRST_SERVER=`echo $SERVERS | cut -d ' ' -f 1`

#if [ -n "$R" ]
#then

#if [ ! -e /dev/shm/loop_db_ssd.img ]
#then
#dd if=/dev/zero of=/dev/shm/loop_db_ssd.img bs=1M count=2000
#sudo losetup /dev/loop0 /dev/shm/loop_db_ssd.img
#fi

#if [ ! -e /dev/shm/loop_wal_ssd.img ]
#then
#dd if=/dev/zero of=/dev/shm/loop_wal_ssd.img bs=1M count=2000
#sudo losetup /dev/loop1 /dev/shm/loop_wal_ssd.img
#fi

HOST_NAME=$HOST"-docker"
HOST_ADDR=`get_server_ip_addr $HOST`

RUN_MON=`is_mon`
RUN_OSD=1
POOL=`get_pool`
OSD_TYPE="filestore"

#DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc -e BS_DB_CREATE=true -e BS_DB_BD=/dev/loop0 -e BS_WAL_CREATE=true -e BS_WAL_BD=/dev/loop1"
#DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc -e BS_DB_CREATE=false -e BS_WAL_CREATE=false"

start

#sleep 5

#if [ ! -e /dev/shm/loop_db_hdd.img ]
#then
#dd if=/dev/zero of=/dev/shm/loop_db_hdd.img bs=1M count=2000
#sudo losetup /dev/loop2 /dev/shm/loop_db_hdd.img
#fi

#if [ ! -e /dev/shm/loop_wal_hdd.img ]
#then
#dd if=/dev/zero of=/dev/shm/loop_wal_hdd.img bs=1M count=2000
#sudo losetup /dev/loop3 /dev/shm/loop_wal_hdd.img
#fi

#HOST_NAME=$HOST"-docker-hdd"
#O=`echo $HOST_ADDR | cut -d . -f 4`
#HOST_ADDR=192.168.0.` expr $O + 1 `

#RUN_MON=`is_mon`
#RUN_OSD=1
#POOL=`get_pool`
#OSD_TYPE="filestore"
#DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb -e BS_DB_CREATE=true -e BS_DB_BD=/dev/loop2 -e BS_WAL_CREATE=true -e BS_WAL_BD=/dev/loop3"
#DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb -e BS_DB_CREATE=false -e BS_WAL_CREATE=false"

#start
#fi
