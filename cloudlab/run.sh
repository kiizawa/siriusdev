#!/bin/sh

set -ex

# Generall Settings

export MASTER_NODE="node-0"
export SLAVE_NODES="node-1 node-2 node-3 node-4 node-5 node-6 node-7 node-8 node-9 node-10 node-11 node-12 node-13 node-14 node-15"
ALL_NODES=$MASTER_NODE" "$SLAVE_NODES
export ALL_NODES

sudo apt-get update
sudo apt install -y dstat
sudo apt install -y sysstat
sudo hostname $MASTER_NODE

for SLAVE_NODE in $SLAVE_NODES
do
    ssh $SLAVE_NODE 'sudo apt-get update'
    ssh $SLAVE_NODE 'sudo apt install -y dstat'
    ssh $SLAVE_NODE 'sudo apt install -y sysstat'
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
    ssh $SLAVE_NODE "sudo mount -t nfs $MASTER_NODE:/tmp/share /tmp/share"
    ssh $SLAVE_NODE "sudo chmod a+w /tmp/share"
done

# Setup overlay network

for NODE in $ALL_NODES
do
    scp setup_overlay_network.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/setup_overlay_network.sh"
done

# Start docker containers

./install_docker_image.sh

#for NODE in $ALL_NODES
#do
#    #scp create_container.sh $NODE:/tmp
#    #ssh $NODE sh -c "/tmp/create_container.sh"
#    scp create_container_pool_based.sh $NODE:/tmp
#    ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
#done
