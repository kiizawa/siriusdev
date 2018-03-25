#!/bin/bash

# Install a docker image on all nodes in parallel

set -ex

NODES="node-0 node-1 node-2 node-3 node-4"

DOCKER_IMAGE=kiizawa/siriusdev

need_to_install=0
for NODE in $NODES
do
    ssh -f $NODE "docker pull $DOCKER_IMAGE"
    need_to_install=`expr $need_to_install + 1`
done

while true
do
    set +e
    installed=0
    for NODE in $NODES
    do
	DONE=`ssh $NODE 'docker images' | grep $DOCKER_IMAGE`
	if [ -n "$DONE" ]
	then
	    echo "docker image installed on $NODE!"
	    installed=`expr $installed + 1`
	fi
done
set -e
if [ $installed -eq $need_to_install ]
then
    echo "docker image installed on all nodes!"
    break
fi
sleep 10
done
