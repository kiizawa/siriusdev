#!/bin/sh

set -x

MASTER_NODE="node-0"
SLAVE_NODES="node-1 node-2 node-3 node-4"
ALL_NODES=$MASTER_NODE" "$SLAVE_NODES

for NODE in $ALL_NODES
do
    scp stop_and_remove_container.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/stop_and_remove_container.sh"
    scp create_container.sh $NODE:/tmp 
    ssh $NODE sh -c "/tmp/create_container.sh"
    #scp create_container_pool_based.sh $NODE:/tmp 
    #ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
done
