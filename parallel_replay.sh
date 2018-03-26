#!/bin/sh

set -ex

READ_PATTERN="p1"
CLIENT_IDS="0"

METHOD=p
HDD_TIER=a
SSD_TIER=s

#METHOD=m
#HDD_TIER=s
#SSD_TIER=f

SHARED_LIST_DIR=/share/XGC_data
THREAD_NUM=128

cp ./replayer.exe /share/

# log

SHARED_LOG_DIR=/share/log
rm -rf $SHARED_LOG_DIR; mkdir $SHARED_LOG_DIR

for i in $CLIENT_IDS
do
    NODE="node-"$i"-docker"
    LOG_DIR=$SHARED_LOG_DIR/$READ_PATTERN
    rm -rf $LOG_DIR; mkdir $LOG_DIR
done

# write (hdd)

for i in $CLIENT_IDS
do
    NODE="node-"$i"-docker"
    W_LIST=$SHARED_LIST_DIR/writer_list/writer.list.$i
    W_LOG=$LOG_DIR/${METHOD}_wh.log.${i}
    ssh -f $NODE "/share/replayer.exe -t $THREAD_NUM -m w -r $HDD_TIER -f $W_LOG -l $W_LIST"
done

# read (hdd)

for i in $CLIENT_IDS
do
    NODE="node-"$i"-docker"
    R_LIST=$SHARED_LIST_DIR/reader_${READ_PATTERN}_list/reader_${READ_PATTERN}.list.$i
    R_LOG=$LOG_DIR/${METHOD}_rh.log.${i}
    ssh -f $NODE "/share/replayer.exe -t $THREAD_NUM -m r -f $R_LOG -l $R_LIST"
done

# move (hdd -> ssd)
for i in $CLIENT_IDS
do
    NODE="node-"$i"-docker"
    R_LIST=$SHARED_LIST_DIR/reader_${READ_PATTERN}_list/reader_${READ_PATTERN}.list.$i
    M_LOG=$LOG_DIR/${METHOD}_ms.log.${i}
    ssh -f $NODE "/share/replayer.exe -t $THREAD_NUM -m m -r $SSD_TIER -f $M_LOG -l $R_LIST"
done

# read (hdd)

for i in $CLIENT_IDS
do
    NODE="node-"$i"-docker"
    R_LIST=$SHARED_LIST_DIR/reader_${READ_PATTERN}_list/reader_${READ_PATTERN}.list.$i
    R_LOG=$LOG_DIR/${METHOD}_rs.log.${i}
    ssh -f $NODE "/share/replayer.exe -t $THREAD_NUM -m r -f $R_LOG -l $R_LIST"
done
