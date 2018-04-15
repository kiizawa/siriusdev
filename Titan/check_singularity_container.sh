#!/bin/bash

sudo singularity exec instance://siriusdev bash -c 'LD_LIBRARY_PATH=/usr/local/lib ceph -s -c /ceph_conf/ceph.conf'
