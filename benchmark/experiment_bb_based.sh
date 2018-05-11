#!/bin/sh

set -ex

# capacity (variable)
CAPACITY=ALL
#CAPACITY=PARTIAL

# system (variable)
NUM_NODES="1+1"
B_SSD=505
B_HDD=179

#NUM_NODES="1+2"
#B_SSD=505
#B_HDD=347

#NUM_NODES="1+4"
#B_SSD=505
#B_HDD=671

# num of patterns (variable)
NUM_PATTERNS=20
PATTERNS=`seq -f %02g 1 $NUM_PATTERNS`
rm -rf file_list
for i in $PATTERNS
do
    echo /tmp/share/XGC_data/reader_synthetic_list/reader_synthetic_list.$i >> file_list
done

# num of readers (fixed)
NUM_READERS=2
READER_IDS=`seq -f %02g 1 $NUM_READERS`

# policy (fixed)
POLICY=LOCALITY_AWARE

###################################################

WRITER_IDS="0"
NUM_WRITERS=`echo $WRITER_IDS | wc -w`

THREAD_NUM=40

METHOD=pool
HDD_TIER=s
SSD_TIER=f

#METHOD=micro
#HDD_TIER=s
#SSD_TIER=f

SHARED_LIST_DIR=/tmp/share/XGC_data

cp ./replayer.exe /tmp/share/
cp ./analyser.exe /tmp/share/

# log

SHARED_LOG_DIR=/tmp/share/log
if [ ! -e $SHARED_LOG_DIR ]
then
    mkdir $SHARED_LOG_DIR
fi

LOG_DIR=$SHARED_LOG_DIR/${NUM_NODES}_${CAPACITY}_${POLICY}
rm -rf $LOG_DIR; mkdir $LOG_DIR

STATS=$LOG_DIR/stats.all

SYNC_FILE=/tmp/share/done
rm -rf $SYNC_FILE

# calcuate data placement

SSD_OBJECTS_LIST=/tmp/share/ssd_set
ALL_OBJECTS_LIST=/tmp/share/working_set
rm -f $SSD_OBJECTS_LIST
rm -f $ALL_OBJECTS_LIST
if [ $POLICY = "LOCALITY_OBLIVIOUS" ]
then
    ./data_placer.exe      -s $B_SSD -d $B_HDD -i file_list -o $SSD_OBJECTS_LIST -w $ALL_OBJECTS_LIST
elif [ $POLICY = "LOCALITY_AWARE" ]
then
    ./data_placer_hint.exe -s $B_SSD -d $B_HDD -i file_list -o $SSD_OBJECTS_LIST -w $ALL_OBJECTS_LIST
fi

# write (hdd)

touch $SYNC_FILE

for i in $WRITER_IDS
do
    if [ $i = "0" ]
    then
	NODE=192.168.0.10
    fi
    W_LIST=$ALL_OBJECTS_LIST
    W_LOG=$LOG_DIR/wh.log.${i}
    ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM -m w -r $HDD_TIER -f $W_LOG -l $W_LIST; echo $i >> $SYNC_FILE"
done

set +ex
while true
do
    if [ `cat $SYNC_FILE | wc -l` -eq $NUM_WRITERS ]
    then
	break
    fi
    sleep 1
done
set -ex
rm -rf $SYNC_FILE

ALL_W_LOG=$LOG_DIR/wh.log.all
for i in $WRITER_IDS
do
    W_LOG=$LOG_DIR/wh.log.${i}
    cat $W_LOG >> $ALL_W_LOG
done
echo "$ALL_W_LOG" >> $STATS
echo "" >> $STATS
./analyser.exe $ALL_W_LOG >> $STATS
echo "" >> $STATS
echo "" >> $STATS

# move (hdd -> ssd)

touch $SYNC_FILE

for i in $WRITER_IDS
do
    if [ $i = "0" ]
    then
	NODE=192.168.0.10
    fi
    if [ $CAPACITY = "ALL" ]
    then
	cat $ALL_OBJECTS_LIST | cut -d , -f 1 > /tmp/share/working_set.move
	M_LIST=/tmp/share/working_set.move
    else
	M_LIST=$SSD_OBJECTS_LIST
    fi
    M_LOG=$LOG_DIR/ms.log.${i}
    ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM -m m -r $SSD_TIER -f $M_LOG -l $M_LIST; echo $i >> $SYNC_FILE"
done

set +ex
while true
do
    if [ `cat $SYNC_FILE | wc -l` -eq $NUM_WRITERS ]
    then
	break
    fi
    sleep 1
done
set -ex
rm -rf $SYNC_FILE

ALL_M_LOG=$LOG_DIR/ms.log.all
for i in $WRITER_IDS
do
    M_LOG=$LOG_DIR/ms.log.${i}
    cat $M_LOG >> $ALL_M_LOG
done
echo "$ALL_M_LOG" >> $STATS
echo "" >> $STATS
./analyser.exe $ALL_M_LOG >> $STATS
echo "" >> $STATS
echo "" >> $STATS

# read

for P in $PATTERNS
do

    touch $SYNC_FILE

    for i in $READER_IDS
    do
	NODE=192.168.0.10
	if [ $i = "01" ]
	then
	    R_LIST=$SHARED_LIST_DIR/reader_synthetic_list/reader_synthetic_list.$P.ssd
	fi
	if [ $i = "02" ]
	then
	    R_LIST=$SHARED_LIST_DIR/reader_synthetic_list/reader_synthetic_list.$P.hdd
	fi
	R_LOG=$LOG_DIR/${P}.log.${i}
	THREAD_NUM_PER_READER=`expr $THREAD_NUM / $NUM_READERS`
	ssh -f $NODE "ulimit -n 4096; /tmp/share/replayer.exe -t $THREAD_NUM_PER_READER -m r -f $R_LOG -l $R_LIST; echo $i >> $SYNC_FILE"
    done

    set +ex
    while true
    do
	if [ `cat $SYNC_FILE | wc -l` -eq $NUM_READERS ]
	then
	    break
	fi
	sleep 1
    done
    rm -rf $SYNC_FILE
    set -ex

    ALL_R_LOG=$LOG_DIR/${P}.log.all
    for i in $READER_IDS
    do
	R_LOG=$LOG_DIR/${P}.log.${i}
	cat $R_LOG >> $ALL_R_LOG
    done

    echo "$ALL_R_LOG" >> $STATS
    echo "" >> $STATS
    ./analyser.exe $ALL_R_LOG >> $STATS
    echo "" >> $STATS
    echo "" >> $STATS

done

cat $STATS | grep elapsed | tail -$NUM_PATTERNS | cut -d ' ' -f 3 > $LOG_DIR/read_time.txt

rm -f $LOG_DIR/read_time2.txt

sum=0
while read line
do
    sum=`expr $sum + $line`
    echo $sum >> $LOG_DIR/read_time2.txt
done < $LOG_DIR/read_time.txt

SUM=`awk '{s += $1} END {print s}' < $LOG_DIR/read_time2.txt`
AVG=`expr $SUM / $NUM_PATTERNS`
echo "avg="$AVG"[ms]" >> $STATS
