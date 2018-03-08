#!/bin/sh

# Generall Settings

sudo apt-get update

MASTER_CLOUDLAB_NODE="c220g2-011027.wisc.cloudlab.us"
SLAVE_NODE_IDS="011005 011002 030632 030631 011130 011126"
SLAVE_CLOUDLAB_NODES=""
for NODE_ID in $SLAVE_NODE_IDS
do
    NODE="c220g2-"$NODE_ID".wisc.cloudlab.us"
    SLAVE_CLOUDLAB_NODES=$SLAVE_CLOUDLAB_NODES" "$NODE
done

# Setup NFS

# 1. master

sudo apt-get install -y nfs-kernel-server
if [ ! -e /opt/nfs ]
then
    sudo mkdir -p /opt/nfs
    sudo su -c "echo '/opt/nfs *(rw,sync,no_subtree_check,no_root_squash)' >> /etc/exports"
    sudo exportfs -ra
    sudo service nfs-kernel-server start
fi

# 2. slave

for SLAVE_NODE in $SLAVE_CLOUDLAB_NODES
do
    ssh $SLAVE_NODE "sudo apt-get install -y nfs-common"
    ssh $SLAVE_NODE "if [ ! -e /tmp/share ]; then mkdir /tmp/share; fi"
    ssh $SLAVE_NODE "sudo mount -t nfs node-0:/opt/nfs /tmp/share"
    ssh $SLAVE_NODE "sudo chmod a+w /tmp/share"
done
