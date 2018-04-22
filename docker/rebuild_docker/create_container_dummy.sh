#!/bin/bash

set -ex

HOST=`hostname`

DOCKER_IMAGE=kiizawa/siriusdev:base6

function start() {
    docker run -it -d --privileged  \
	--name $HOST_NAME \
	$DOCKER_IMAGE /bin/bash
}

HOST_NAME=$HOST"-docker"

start
