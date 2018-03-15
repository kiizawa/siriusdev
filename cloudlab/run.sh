#!/bin/sh

set -ex

# Generall Settings

MASTER_NODE="node-0"
SLAVE_NODES="node-1 node-2 node-3 node-4"
ALL_NODES=$MASTER_NODE" "$SLAVE_NODES

sudo apt-get update
sudo hostname $MASTER_NODE

for SLAVE_NODE in $SLAVE_NODES
do
    ssh $SLAVE_NODE 'sudo apt-get update'
    ssh $SLAVE_NODE 'sudo hostname `hostname | cut -d . -f 1`'
done

# Setup NFS

## 1. master

sudo apt-get install -y nfs-kernel-server
if [ ! -e /tmp/share ]
then
    sudo mkdir -p /tmp/share
    sudo su -c "echo '/tmp/share *(rw,sync,no_subtree_check,no_root_squash)' >> /etc/exports"
    sudo exportfs -ra
    sudo service nfs-kernel-server start
fi

## 2. slave

for SLAVE_NODE in $SLAVE_NODES
do
    ssh $SLAVE_NODE "sudo apt-get install -y nfs-common"
    ssh $SLAVE_NODE "if [ ! -e /tmp/share ]; then mkdir /tmp/share; fi"
    ssh $SLAVE_NODE "sudo mount -t nfs node-0:/tmp/share /tmp/share"
    ssh $SLAVE_NODE "sudo chmod a+w /tmp/share"
done

# Setup overlay network

for NODE in $ALL_NODES
do
    scp setup_overlay_network.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/setup_overlay_network.sh"
done

# Start docker containers

for NODE in $ALL_NODES
do
    scp create_container.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/create_container.sh"
    #scp create_container_pool_based.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
done