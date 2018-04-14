#!/bin/sh

set -x

SHARE_DIR=/tmp/share
ROLES_FILE=$SHARE_DIR/roles
IP_ADDR_FILE=$SHARE_DIR/ip_addr
rm -f $ROLES_FILE $IP_ADDR_FILE

CLIENTS=""
SERVERS="node-0 node-1"
ALL_NODES=$CLIENTS" "$SERVERS

for NODE in $ALL_NODES
do
    scp stop_and_remove_singularity_container.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/stop_and_remove_singularity_container.sh"
done

for NODE in $ALL_NODES
do
    #scp create_container.sh $NODE:/tmp
    #ssh $NODE sh -c "/tmp/create_container.sh"
    scp create_singularity_container.sh $NODE:/tmp
    scp role_decider.sh $NODE:/tmp
    ssh $NODE sh -c "/tmp/create_singularity_container.sh"
done
