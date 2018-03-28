#!/bin/sh

set -x

CLIENTS="node-0 node-1"
SERVERS="node-5 node-6 node-7 node-8"
ALL_NODES=$CLIENTS" "$SERVERS

for NODE in $ALL_NODES
do
    #scp stop_and_remove_container.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/stop_and_remove_container.sh"
    #scp create_container.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container.sh"
    scp create_container_pool_based.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
done
