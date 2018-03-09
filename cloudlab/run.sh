#!/bin/sh

# Generall Settings

MASTER_NODE="node-0"
SLAVE_NODES="node-1 node-2 node-3 node-4 node-5 node-6"

sudo apt-get update
sudo sh -c 'hostname $MASTER_NODE'

for SLAVE_NODE in $SLAVE_NODES
do
    ssh $SLAVE_NODE sudo sh -c 'apt-get update'
    ssh $SLAVE_NODE 'sudo hostname `hostname | cut -d . -f 1`'
done

# Setup NFS

## 1. master

sudo apt-get install -y nfs-kernel-server
if [ ! -e /opt/nfs ]
then
    sudo mkdir -p /opt/nfs
    sudo su -c "echo '/opt/nfs *(rw,sync,no_subtree_check,no_root_squash)' >> /etc/exports"
    sudo exportfs -ra
    sudo service nfs-kernel-server start
fi

## 2. slave

for SLAVE_NODE in $SLAVE_NODES
do
    ssh $SLAVE_NODE "sudo apt-get install -y nfs-common"
    ssh $SLAVE_NODE "if [ ! -e /tmp/share ]; then mkdir /tmp/share; fi"
    ssh $SLAVE_NODE "sudo mount -t nfs node-0:/opt/nfs /tmp/share"
    ssh $SLAVE_NODE "sudo chmod a+w /tmp/share"
done

# Setup etcd

for SLAVE_NODE in $SLAVE_NODES
do
    scp setup_overlay_network.sh $SLAVE_NODE:/tmp
    ssh $SLAVE_NODE sudo sh -c '/tmp/setup_overlay_network.sh'
done
