#!/bin/bash

SINGULARITY_IMAGE=/ccs/home/kiizawa/titan_official.img
CEPH_CONF_DIR=$PROJWORK/csc143/$USER/ceph/conf

singularity exec $SINGULARITY_IMAGE bash -c 'LD_LIBRARY_PATH=/usr/local/lib ceph -s -c $CEPH_CONF_DIR/ceph.conf'
