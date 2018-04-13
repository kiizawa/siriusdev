#!/bin/sh

set -x

CLIENTS="node-0 node-1 node-2"
ALL_NODES=$CLIENTS" "$SERVERS

for NODE in $ALL_NODES
do
    scp install_singularity.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/install_singularity.sh"
done
