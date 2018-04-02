#!/bin/sh

WORK_DIR=/dev/shm
DOCKER_IMAGE=kiizawa/siriusdev
HOME=/home/$USER

cd $WORK_DIR
mkdir cache
mkdir tmp
mkdir localcache

export $HOME
export SINGULARITY_CACHEDIR=$WORK_DIR/cache
export SINGULARITY_TMPDIR=$WORK_DIR/tmp 
export SINGULARITY_LOCALCACHEDIR=$WORK_DIR/localcache

singularity create siriusdev.img
singularity build siriusdev.img docker://$DOCKER_IMAGE
