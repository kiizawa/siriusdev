#!/bin/sh

set -x

ALL_NODES="node-0 node-1 node-2 node-3 node-4 node-5 node-6 node-7 node-8 node-9 node-10 node-11 node-12 node-13 node-14"

for NODE in $ALL_NODES
do
    #scp stop_and_remove_container.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/stop_and_remove_container.sh"
    scp create_container.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/create_container.sh"
    #scp create_container_pool_based.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container_pool_based.sh"
done
