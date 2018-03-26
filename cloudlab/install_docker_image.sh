#!/bin/bash

# Install a docker image on all nodes in parallel

set -ex

NODES="node-0 node-1 node-2 node-3 node-4 node-5 node-6 node-7 node-8 node-9 node-10 node-11 node-12 node-13 node-14 node-15"

DOCKER_IMAGE=kiizawa/siriusdev:ssh

need_to_install=0
for NODE in $NODES
do
    ssh -f $NODE "docker pull $DOCKER_IMAGE"
    need_to_install=`expr $need_to_install + 1`
done

set +e
while true
do
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
