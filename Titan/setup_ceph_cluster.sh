#!/bin/sh

# number of OSDs that store objects in memory
NUM_SSD_NODES=5

# number of OSDs that store objects in NFS mounted directory
NUM_TD_NODES=10

# number of OSDs that store objects in HPSS (not supported yet)
NUM_TD_NODES=0

# directory where ceph.conf is generated (needs to be shared)
CEPH_CONF_DIR=$PROJWORK/csc143/$USER/ceph/conf
if [ ! -e $CEPH_CONF_DIR ]
then
    mkdir -p $CEPH_CONF_DIR
else
    rm -rf $CEPH_CONF_DIR/*
fi

CEPH_DATA_DIR=$PROJWORK/csc143/$USER/ceph/data
if [ ! -e $CEPH_DATA_DIR ]
then
    mkdir -p $CEPH_DATA_DIR
else
    rm -rf $CEPH_DATA_DIR/*
fi

CEPH_SCRIPTS_DIR=$PROJWORK/csc143/$USER/ceph/scripts
if [ ! -e $CEPH_SCRIPTS_DIR ]
then
    mkdir -p $CEPH_SCRIPTS_DIR
else
    rm -rf $CEPH_SCRIPTS_DIR/*
fi

cp create_singularity_container.sh check_singularity_container.sh role_decider.sh $CEPH_SCRIPTS_DIR/

rm -f $CEPH_SCRIPTS_DIR/ip_addr
rm -f $CEPH_SCRIPTS_DIR/roles
rm -f $CEPH_SCRIPTS_DIR/setting

echo "NUM_SSD_NODES=$NUM_SSD_NODES" >> $CEPH_SCRIPTS_DIR/setting
echo "NUM_HDD_NODES=$NUM_HDD_NODES" >> $CEPH_SCRIPTS_DIR/setting
echo "NUM_TD_NODES=$NUM_TD_NODES"   >> $CEPH_SCRIPTS_DIR/setting

NUM_NODES=`expr $NUM_SSD_NODES + $NUM_HDD_NODES + $NUM_TD_NODES`
for i in `seq 1 $NUM_NODES`
do
    qsub setup.pbs
done

qsub check.pbs
