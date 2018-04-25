#!/bin/sh

set -ex

READ_PATTERN="p3"
CLIENT_IDS="0"
THREAD_NUM=16
NUM_CLIENTS=`echo $CLIENT_IDS | wc -w`
NUM_NODES=2

METHOD=pool
HDD_TIER=s
SSD_TIER=f

#METHOD=micro
#HDD_TIER=s
#SSD_TIER=f

SHARED_LIST_DIR=/tmp/share/XGC_data

cp ./replayer.exe /tmp/share/

# log

SHARED_LOG_DIR=/tmp/share/log
if [ ! -e $SHARED_LOG_DIR ]
then
    mkdir $SHARED_LOG_DIR
fi

LOG_DIR=$SHARED_LOG_DIR/${NUM_NODES}_${METHOD}_${READ_PATTERN}
rm -rf $LOG_DIR; mkdir $LOG_DIR

STATS=$LOG_DIR/stats.all

SYNC_FILE=/tmp/share/done
rm -rf $SYNC_FILE

# write (hdd)

touch $SYNC_FILE

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
    W_LIST=$SHARED_LIST_DIR/reader_p3_list/reader_p3_list_unq_u
    W_LOG=$LOG_DIR/wh.log.${i}
    ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM -m w -r $HDD_TIER -f $W_LOG -l $W_LIST; echo $i >> $SYNC_FILE"
done

set +ex
while true
do
    if [ `cat $SYNC_FILE | wc -l` -eq $NUM_CLIENTS ]
    then
	break
    fi
    sleep 1
done
set -ex
rm -rf $SYNC_FILE

ALL_W_LOG=$LOG_DIR/wh.log.all
for i in $CLIENT_IDS
do
    W_LOG=$LOG_DIR/wh.log.${i}
    cat $W_LOG >> $ALL_W_LOG
done
echo "$ALL_W_LOG" >> $STATS
echo "" >> $STATS
./analyser.exe $ALL_W_LOG >> $STATS
echo "" >> $STATS
echo "" >> $STATS

# read (hdd)

touch $SYNC_FILE

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
    R_LIST=$SHARED_LIST_DIR/paper/reader_p3_list_unq
    R_LOG=$LOG_DIR/rh.log.${i}
    ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM -m r -f $R_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
done

set +ex
while true
do
    if [ `cat $SYNC_FILE | wc -l` -eq $NUM_CLIENTS ]
    then
	break
    fi
    sleep 1
done
set -ex
rm -rf $SYNC_FILE

ALL_R_LOG=$LOG_DIR/rh.log.all
for i in $CLIENT_IDS
do
    R_LOG=$LOG_DIR/rh.log.${i}
    cat $R_LOG >> $ALL_R_LOG
done
echo "$ALL_R_LOG" >> $STATS
echo "" >> $STATS
./analyser.exe $ALL_R_LOG >> $STATS
echo "" >> $STATS
echo "" >> $STATS

# move (hdd -> ssd)

touch $SYNC_FILE

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
    R_LIST=$SHARED_LIST_DIR/paper/reader_p3_list_unq
    M_LOG=$LOG_DIR/ms.log.${i}
    ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM -m m -r $SSD_TIER -f $M_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
done

set +ex
while true
do
    if [ `cat $SYNC_FILE | wc -l` -eq $NUM_CLIENTS ]
    then
	break
    fi
    sleep 1
done
set -ex
rm -rf $SYNC_FILE

ALL_M_LOG=$LOG_DIR/ms.log.all
for i in $CLIENT_IDS
do
    M_LOG=$LOG_DIR/ms.log.${i}
    cat $M_LOG >> $ALL_M_LOG
done
echo "$ALL_M_LOG" >> $STATS
echo "" >> $STATS
./analyser.exe $ALL_M_LOG >> $STATS
echo "" >> $STATS
echo "" >> $STATS

# read (ssd)

touch $SYNC_FILE

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
    R_LIST=$SHARED_LIST_DIR/paper/reader_p3_list_unq
    R_LOG=$LOG_DIR/rs.log.${i}
    ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM -m r -f $R_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
done

set +e
while true
do
    if [ `cat $SYNC_FILE | wc -l` -eq $NUM_CLIENTS ]
    then
	break
    fi
    sleep 1
done
rm -rf $SYNC_FILE
set -e

ALL_R_LOG=$LOG_DIR/rs.log.all
for i in $CLIENT_IDS
do
    R_LOG=$LOG_DIR/rs.log.${i}
    cat $R_LOG >> $ALL_R_LOG
done
echo "$ALL_R_LOG" >> $STATS
echo "" >> $STATS
./analyser.exe $ALL_R_LOG >> $STATS
echo "" >> $STATS
echo "" >> $STATS
