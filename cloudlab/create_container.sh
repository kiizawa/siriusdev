#!/bin/bash

set -ex

DOCKER_IMAGE=kiizawa/siriusdev

CEPH_NET=175.20.0.0/16

#netexists=`docker network ls --filter name=cephnet -q | wc -l`
#if [ $netexists -eq 0 ]; then
#  docker network create --subnet=$CEPH_NET cephnet
#fi

docker run -it -d --privileged  \
  --name $HOST_NAME \
  --net cephnet \
  --hostname $HOST_NAME \
  --ip $HOST_ADDR \
  -v `pwd`/share:/share \
  -v `pwd`/src:/src \
  -e CEPH_CONF_DIR=/share \
  -e RUN_MON=$RUN_MON \
  -e RUN_OSD=1 \
  -e OSD_TYPE=$OSD_TYPE $DEVICE_ARGS \
  -e POOL=$POOL \
  -e CEPH_PUBLIC_NETWORK=$CEPH_NET \
  $DOCKER_IMAGE
