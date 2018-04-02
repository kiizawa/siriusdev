#!/bin/bash

IMAGE=/dev/shm/siriusdev.img

HOME_DIR=/home/dummy
if [ ! -e $HOME_DIR ]
then
    mkdir $HOME_DIR
fi

sudo singularity exec -H $HOME_DIR $IMAGE /bin/bash
