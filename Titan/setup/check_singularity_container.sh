#!/bin/bash

source $MODULESHOME/init/bash
module load singularity

SINGULARITY_IMAGE=$PROJWORK/csc143/ceph/ceph_on_titan_v1.img
CEPH_CONF_DIR=$PROJWORK/csc143/$USER/ceph/conf

SINGULARITYENV_CEPH_CONF_DIR=$CEPH_CONF_DIR \
    singularity exec $SINGULARITY_IMAGE /root/check_ceph.sh
