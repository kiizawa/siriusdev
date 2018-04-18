#!/bin/sh

NUM_SSD_NODES=0
NUM_HDD_NODES=1
NUM_TD_NODES=0
TIME=00:01:00

CEPH_CONF_DIR=$PROJWORK/csc143/$USER/ceph/conf
if [ ! -e $CEPH_CONF_DIR ]
then
    mkdir -p $CEPH_CONF_DIR
else
    rm -rf $CEPH_CONF_DIR/*
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
qsub -l nodes=$NUM_NODES -l walltime=$TIME setup.pbs
