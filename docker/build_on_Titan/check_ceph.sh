#!/bin/sh

set -ex

if [ -z "$CEPH_CONF_DIR" ]
then
      echo "CEPH_CONF_DIR must be specified!"
      exit
fi

CEPH_CONF=$CEPH_CONF_DIR/ceph.conf
CEPH_BIN_DIR=/usr/local/bin
CEPH_LIB_DIR=/usr/local/lib

export LD_LIBRARY_PATH=$CEPH_LIB_DIR

CONF_ARGS="--conf $CEPH_CONF"

$CEPH_BIN_DIR/ceph -s $CONF_ARGS
