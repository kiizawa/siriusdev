#!/bin/sh

WORK_DIR=/dev/shm
DOCKER_IMAGE=kiizawa/siriusdev:memstore_journal_xattr_monip_dummydir4
SINGULARITY_IMAGE=/dev/shm/siriusdev.img
SANDBOX_DIR=$WORK_DIR/siriusdev_sandbox_dir

export SINGULARITY_CACHEDIR=$WORK_DIR/cache
export SINGULARITY_TMPDIR=$WORK_DIR/tmp 
export SINGULARITY_LOCALCACHEDIR=$WORK_DIR/localcache

rm -rf $SINGULARITY_CACHEDIR; mkdir $SINGULARITY_CACHEDIR
rm -rf $SINGULARITY_TMPDIR; mkdir $SINGULARITY_TMPDIR
rm -rf $SINGULARITY_LOCALCACHEDIR; mkdir $SINGULARITY_LOCALCACHEDIR
rm -rf $SANDBOX_DIR

singularity build --sandbox $SANDBOX_DIR docker://$DOCKER_IMAGE
singularity build --writable $SINGULARITY_IMAGE $SANDBOX_DIR
