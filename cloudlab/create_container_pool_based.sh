#!/bin/bash

set -ex

DOCKER_IMAGE=kiizawa/siriusdev:ssh_pg_log_bd
CLIENTS="node-0 node-1 node-2"
SERVERS=" node-3 node-4 node-5 node-6 node-7 node-8"

get_client_ip_addr () {
    HOST=$1
    OFFSET=10
    for i in $CLIENTS
    do
	if [ $i = $HOST ]
	then
	    break
	else
	    OFFSET=` expr $OFFSET + 1 `
	fi
    done
    echo "192.168.0.$OFFSET"
}

get_server_ip_addr () {
    HOST=$1
    OFFSET=10
    for i in $CLIENTS
    do
	OFFSET=` expr $OFFSET + 1 `
    done
    for i in $SERVERS
    do
	if [ $i = $HOST ]
	then
	    break
	else
	    OFFSET=` expr $OFFSET + 2 `
	fi
    done
    echo "192.168.0.$OFFSET"
}

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
	-e METHOD=$METHOD \
	-e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
	-e POOL=$POOL \
	-e CEPH_PUBLIC_NETWORK=$CEPH_NET \
	$CONFIG_OPTS \
	$DOCKER_IMAGE
}

CEPH_NET=192.168.0.0/16
METHOD="pool"
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
    HOST_ADDR=`get_client_ip_addr $HOST`
    RUN_MON=0
    RUN_OSD=0
    start
    exit
fi

# servers

set +e
R=`echo $SERVERS | grep $HOST`
set -e

FIRST_SERVER=`echo $SERVERS | cut -d ' ' -f 1`

if [ -n "$R" ]
then

    if [ ! -e /dev/shm/loop_db_ssd.img ]
    then
	dd if=/dev/zero of=/dev/shm/loop_db_ssd.img bs=1M count=2000
	sudo losetup /dev/loop0 /dev/shm/loop_db_ssd.img
    fi

    if [ ! -e /dev/shm/loop_wal_ssd.img ]
    then
	dd if=/dev/zero of=/dev/shm/loop_wal_ssd.img bs=1M count=2000
	sudo losetup /dev/loop1 /dev/shm/loop_wal_ssd.img
    fi

    HOST_NAME=$HOST"-docker-ssd"
    HOST_ADDR=`get_server_ip_addr $HOST`
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
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdc -e BS_DB_CREATE=true -e BS_DB_BD=/dev/loop0 -e BS_WAL_CREATE=true -e BS_WAL_BD=/dev/loop1"
    start

    sleep 5

    if [ ! -e /dev/shm/loop_db_hdd.img ]
    then
	dd if=/dev/zero of=/dev/shm/loop_db_hdd.img bs=1M count=2000
	sudo losetup /dev/loop2 /dev/shm/loop_db_hdd.img
    fi

    if [ ! -e /dev/shm/loop_wal_hdd.img ]
    then
	dd if=/dev/zero of=/dev/shm/loop_wal_hdd.img bs=1M count=2000
	sudo losetup /dev/loop3 /dev/shm/loop_wal_hdd.img
    fi

    HOST_NAME=$HOST"-docker-hdd"
    O=`echo $HOST_ADDR | cut -d . -f 4`
    HOST_ADDR=192.168.0.` expr $O + 1 `
    RUN_MON=0
    RUN_OSD=1
    POOL="archive_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_CREATE=false -e BS_SLOW_BD=/dev/sdb -e BS_DB_CREATE=true -e BS_DB_BD=/dev/loop2 -e BS_WAL_CREATE=true -e BS_WAL_BD=/dev/loop3"
    start
fi
