#!/bin/sh

set -ex

READ_PATTERN="p1"
CLIENT_IDS="0"

METHOD=pool
HDD_TIER=a
SSD_TIER=s

#METHOD=micro
#HDD_TIER=s
#SSD_TIER=f

SHARED_LIST_DIR=/share/XGC_data
THREAD_NUM=128

cp ./replayer.exe /share/

# log

SHARED_LOG_DIR=/share/log
rm -rf $SHARED_LOG_DIR; mkdir $SHARED_LOG_DIR

LOG_DIR=$SHARED_LOG_DIR/$READ_PATTERN
rm -rf $LOG_DIR; mkdir $LOG_DIR

SYNC_FILE=/share/done

# write (hdd)

for i in $CLIENT_IDS
do
    if [ $i = "0" ]
    then
	NODE=192.168.0.10
    fi
    if [ $i = "1" ]
    then
	NODE=192.168.0.11
    fi
    if [ $i = "2" ]
    then
	NODE=192.168.0.12
    fi
    if [ $i = "3" ]
    then
	NODE=192.168.0.13
    fi
    if [ $i = "4" ]
    then
	NODE=192.168.0.14
    fi
    W_LIST=$SHARED_LIST_DIR/writer_list/writer.list.$i
    W_LOG=$LOG_DIR/${METHOD}_wh.log.${i}
    ssh -f $NODE "ulimit -n 4096; /share/replayer.exe -t $THREAD_NUM -m w -r $HDD_TIER -f $W_LOG -l $W_LIST; echo $i >> $SYNC_FILE"
done

exit

# read (hdd)

for i in $CLIENT_IDS
do
    if [ $i = "0" ]
    then
	NODE=192.168.0.10
    fi
    if [ $i = "1" ]
    then
	NODE=192.168.0.11
    fi
    if [ $i = "2" ]
    then
	NODE=192.168.0.12
    fi
    if [ $i = "3" ]
    then
	NODE=192.168.0.13
    fi
    if [ $i = "4" ]
    then
	NODE=192.168.0.14
    fi
    R_LIST=$SHARED_LIST_DIR/reader_${READ_PATTERN}_list/reader_${READ_PATTERN}.list.$i
    R_LOG=$LOG_DIR/${METHOD}_rh.log.${i}
    ssh -f $NODE "ulimit -n 4096; /share/replayer.exe -t $THREAD_NUM -m r -f $R_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
done

# move (hdd -> ssd)
for i in $CLIENT_IDS
do
    if [ $i = "0" ]
    then
	NODE=192.168.0.10
    fi
    if [ $i = "1" ]
    then
	NODE=192.168.0.11
    fi
    if [ $i = "2" ]
    then
	NODE=192.168.0.12
    fi
    if [ $i = "3" ]
    then
	NODE=192.168.0.13
    fi
    if [ $i = "4" ]
    then
	NODE=192.168.0.14
    fi
    R_LIST=$SHARED_LIST_DIR/reader_${READ_PATTERN}_list/reader_${READ_PATTERN}.list.$i
    M_LOG=$LOG_DIR/${METHOD}_ms.log.${i}
    ssh -f $NODE "ulimit -n 4096; /share/replayer.exe -t $THREAD_NUM -m m -r $SSD_TIER -f $M_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
done

# read (ssd)

for i in $CLIENT_IDS
do
    if [ $i = "0" ]
    then
	NODE=192.168.0.10
    fi
    if [ $i = "1" ]
    then
	NODE=192.168.0.11
    fi
    if [ $i = "2" ]
    then
	NODE=192.168.0.12
    fi
    if [ $i = "3" ]
    then
	NODE=192.168.0.13
    fi
    if [ $i = "4" ]
    then
	NODE=192.168.0.14
    fi
    R_LIST=$SHARED_LIST_DIR/reader_${READ_PATTERN}_list/reader_${READ_PATTERN}.list.$i
    R_LOG=$LOG_DIR/${METHOD}_rs.log.${i}
    ssh -f $NODE "ulimit -n 4096; /share/replayer.exe -t $THREAD_NUM -m r -f $R_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
done
