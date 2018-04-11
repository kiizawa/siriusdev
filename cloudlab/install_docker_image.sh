#!/bin/bash

# Install a docker image on all nodes in parallel

set -ex

DOCKER_IMAGE=kiizawa/siriusdev:memstore_journal_monip

need_to_install=0
for NODE in $ALL_NODES
do
    ssh -f $NODE "docker pull $DOCKER_IMAGE"
    need_to_install=`expr $need_to_install + 1`
done

while true
do
    installed=0
    set +e
    for NODE in $ALL_NODES
    do
	DONE=`ssh $NODE 'docker images $DOCKER_IMAGE' | grep -v TAG`
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
