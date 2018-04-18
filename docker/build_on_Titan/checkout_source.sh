#!/bin/sh

git clone git://github.com/ceph/ceph && \
    cd ceph && \
    git fetch origin pull/18211/head:wip_bluestore_tiering && \
    git checkout wip_bluestore_tiering

git clone https://github.com/fbarriga/fuse_xattrs
