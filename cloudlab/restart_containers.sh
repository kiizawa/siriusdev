#!/bin/sh

set -x

CLIENTS="node-0"
SERVERS="node-1 node-2 node-3"
ALL_NODES=$CLIENTS" "$SERVERS

for NODE in $ALL_NODES
do
    scp stop_and_remove_container.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/stop_and_remove_container.sh"
done

for NODE in $ALL_NODES
do
    #scp create_container_micro.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container_micro.sh"
    #scp create_container_pool_based.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
    scp create_container_pool_based_separate_nodes.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/create_container_pool_based_separate_nodes.sh"
done
