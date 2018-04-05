#!/bin/sh

set -ex

CEPH_CONF=$CEPH_CONF_DIR/ceph.conf
CEPH_BIN_DIR=/usr/local/bin
CEPH_LIB_DIR=/usr/local/lib

export LD_LIBRARY_PATH=$CEPH_LIB_DIR

CONF_ARGS="--conf $CEPH_CONF"

#########
## MON ##
#########

if [ -n "$RUN_MON" -a $RUN_MON = 1 ]
then

if [ -z "$POOL_SIZE" ]
then
POOL_SIZE=2
fi

if [ -z "$PG_NUM" ]
then
PG_NUM=8
fi

if [ -z "$LOG_DIR" ]
then
LOG_DIR=/tmp/ceph
fi

FSID=`uuidgen`
MON_DATA_DIR=/tmp/ceph/mon_data

rm -f $CEPH_CONF
cat <<EOF >> $CEPH_CONF
[global]

fsid = $FSID

mon initial members = $HOSTNAME
mon host = `hostname -i`
public network = $CEPH_PUBLIC_NETWORK

auth cluster required = none
auth service required = none
auth client required = none
osd journal size = 1024
filestore xattr use omap = true
osd pool default size = $POOL_SIZE
osd pool default min size = 1
osd pool default pg num = $PG_NUM
osd pool default pgp num = $PG_NUM
osd crush chooseleaf type = 1
ms_bind_ipv6 = false

log file = $LOG_DIR/\$name.log

[client]
keyring = /etc/ceph/keyring

[mon]
mon data = $MON_DATA_DIR
mon max pg per osd = 800

EOF

# create keyring

$CEPH_BIN_DIR/ceph-authtool $CEPH_CONF_DIR/keyring --gen-key --name mon.0 --cap mon 'allow *' --create-keyring
$CEPH_BIN_DIR/ceph-authtool $CEPH_CONF_DIR/keyring --gen-key --name client.admin --set-uid=0 --cap mon 'allow *' --cap osd 'allow *'

# create monmap

$CEPH_BIN_DIR/monmaptool --create --clobber --add $HOSTNAME `hostname -i` --fsid $FSID $CEPH_CONF_DIR/monmap

# crate mon data directory

rm -rf $MON_DATA_DIR; mkdir -p $MON_DATA_DIR

# populate monitor daemons

$CEPH_BIN_DIR/ceph-mon --mkfs -i $HOSTNAME --monmap $CEPH_CONF_DIR/monmap --keyring $CEPH_CONF_DIR/keyring $CONF_ARGS

# setup mgr daemons

$CEPH_BIN_DIR/ceph-authtool $CEPH_CONF_DIR/keyring --gen-key --name mgr.0 --cap mon 'allow profile mgr' --cap osd 'allow *'

# start mon daemons

$CEPH_BIN_DIR/ceph-mon -i $HOSTNAME --pid-file $MON_DATA_DIR/mon.$HOSTNAME.pid --cluster ceph $CONF_ARGS

# start mgr daemons

$CEPH_BIN_DIR/ceph-mgr -i 0 $CONF_ARGS

# create crush rules

$CEPH_BIN_DIR/ceph osd setcrushmap -i /root/crushmap $CONF_ARGS

# create pool

$CEPH_BIN_DIR/ceph osd pool create storage_pool $PG_NUM $KEY_ARGS storage_pool_rule $CONF_ARGS
$CEPH_BIN_DIR/ceph osd pool create archive_pool $PG_NUM $KEY_ARGS archive_pool_rule $CONF_ARGS

fi

#########
## OSD ##
#########

if [ -n "$RUN_OSD" -a $RUN_OSD = 1 ]
then
if [ -z "$OSD_DATA_DIR" ]
then
OSD_DATA_DIR=/tmp/ceph/osd_data
fi
if [ -z "$OSD_JOURNAL" ]
then
OSD_JOURNAL=$OSD_DATA_DIR"/journal"
fi
osd_num=`$CEPH_BIN_DIR/ceph osd create $CONF_ARGS`

set +e
osd_entry=`cat $CEPH_CONF | grep "\[osd\]"`
set -e

if [ -z "$osd_entry" ]
then

if [ -z "$OP_THREADS" ]
then
OP_THREADS=2
fi

ROCKSDB_CACHE_FLAG=true
if [ -z "$BS_CACHE_SIZE" ]
then
BS_CACHE_SIZE_HDD=1G
BS_CACHE_SIZE_SSD=3G
else
BS_CACHE_SIZE_HDD=$BS_CACHE_SIZE
BS_CACHE_SIZE_SSD=$BS_CACHE_SIZE
if [ $BS_CACHE_SIZE -eq 0 ]
then
ROCKSDB_CACHE_FLAG=false
fi
fi

cat <<EOF >> $CEPH_CONF
[osd]
osd data = $OSD_DATA_DIR
osd journal = $OSD_JOURNAL
osd op threads = $OP_THREADS
bluestore cache size hdd = $BS_CACHE_SIZE_HDD
bluestore cache size ssd = $BS_CACHE_SIZE_SSD
rocksdb_cache_index_and_filter_blocks = $ROCKSDB_CACHE_FLAG
rocksdb_pin_l0_filter_and_index_blocks_in_cache = $ROCKSDB_CACHE_FLAG
debug ms = 1
debug osd = 25
debug objecter = 20
debug monc = 20
debug mgrc = 20
debug journal = 2
debug filestore = 20
debug bluestore = 30
debug bluefs = 20
debug rocksdb = 10
debug bdev = 20
debug reserver = 10
debug objclass = 20

EOF
fi

if [ $OSD_TYPE = "bluestore" ]
then
if [ -z "$BS_FAST_CREATE" ]
then
BS_FAST_CREATE=false
fi
if [ -z "$BS_DB_CREATE" ]
then
BS_DB_CREATE=false
fi
if [ -z "$BS_WAL_CREATE" ]
then
BS_WAL_CREATE=false
fi

cat <<EOF >> $CEPH_CONF
[osd.$osd_num]
host = `hostname -s`
osd objectstore = bluestore
bluestore block create = true
bluestore block path = $BS_SLOW_BD
bluestore block fast create = $BS_FAST_CREATE
bluestore block fast path = $BS_FAST_BD
bluestore block db create = $BS_DB_CREATE
bluestore block db path = $BS_DB_BD
bluestore block wal create = $BS_WAL_CREATE
bluestore block wal path = $BS_WAL_BD
bluestore mkfs on mount = true
bluestore fsck on mount = false
;bluefs preextend wal files = true
;bluestore default buffered read = false

EOF
else
cat <<EOF >> $CEPH_CONF
[osd.$osd_num]
host = `hostname -s`
osd objectstore = filestore

EOF
fi

# create osd data directory

if [ -e $OSD_DATA_DIR ]
then
    rm -rf $OSD_DATA_DIR/*
else
    mkdir -p $OSD_DATA_DIR
fi

# initialize journal

rm -f $OSD_JOURNAL

KEY_ARGS="--keyring $CEPH_CONF_DIR/keyring"

$CEPH_BIN_DIR/ceph-osd -i $osd_num --mkfs $CONF_ARGS
$CEPH_BIN_DIR/ceph-authtool $CEPH_CONF_DIR/keyring --gen-key --name osd.$osd_num --cap osd 'allow w' --cap mon 'allow rwx' $CONF_ARGS

$CEPH_BIN_DIR/ceph osd crush add osd.$osd_num 1.0 host=$HOSTNAME rack=$POOL $KEY_ARGS $CONF_ARGS

# start osd daemons

$CEPH_BIN_DIR/ceph-osd -i $osd_num $CONF_ARGS

fi

# start sshd

service ssh start

while true
do 
    sleep 3600
done
