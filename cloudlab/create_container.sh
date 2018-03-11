#!/bin/bash

set -ex

DOCKER_IMAGE=kiizawa/siriusdev

CEPH_NET=192.168.0.0/16

HOST_NAME=`hostname`"-docker"

if [ `hostname` = "node-0" ]
then
    HOST_ADDR="192.168.0.10"
    RUN_MON=0
    RUN_OSD=0
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
fi

if [ `hostname` = "node-1" ]
then
    HOST_ADDR="192.168.0.11"
    RUN_MON=1
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
fi

if [ `hostname` = "node-2" ]
then
    HOST_ADDR="192.168.0.12"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
fi

if [ `hostname` = "node-3" ]
then
    HOST_ADDR="192.168.0.13"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
fi

if [ `hostname` = "node-4" ]
then
    HOST_ADDR="192.168.0.14"
    RUN_MON=0
    RUN_OSD=1
    POOL="storage_pool"
    OSD_TYPE="bluestore"
    DEVICE_ARGS="-e BS_FAST_BD=/dev/sdc -e BS_SLOW_BD=/dev/sdb"
fi

docker run -it -d --privileged  \
  --name $HOST_NAME \
  --net cephnet \
  --hostname $HOST_NAME \
  --ip $HOST_ADDR \
  -v /tmp/share:/share \
  -e CEPH_CONF_DIR=/share \
  -e RUN_MON=$RUN_MON \
  -e RUN_OSD=$RUN_OSD \
  -e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
  -e POOL=$POOL \
  -e CEPH_PUBLIC_NETWORK=$CEPH_NET \
  $DOCKER_IMAGE
