#!/bin/sh

set -x

CLIENTS="node-0"
SERVERS="node-1 node-2"
ALL_NODES=$CLIENTS" "$SERVERS

for NODE in $ALL_NODES
do
    scp stop_and_remove_container.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/stop_and_remove_container.sh"
done

for NODE in $ALL_NODES
do
    #scp create_container.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container.sh"
    scp create_container_pool_based.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
done
