#!/bin/sh

set -x

NUM_SSD=1
NUM_HDD=1
#NUM_TD=1

SHARE_DIR=/tmp/share
LOCK_FILE=$SHARE_DIR/lock
ROLES_FILE=$SHARE_DIR/roles
IP_ADDR_FILE=$SHARE_DIR/ip_addr

is_mon()
{
    MON_RUNNING=`cat $ROLES_FILE | grep mon`
    if [ -z "$MON_RUNNING" ]
    then
	echo 1
	echo mon" "`hostname -i` >> $ROLES_FILE
    else
	echo 0
    fi
}

get_pool()
{
    NS=`cat $ROLES_FILE | grep ssd | wc -l`
    if [ $NS -lt $NUM_SSD ]
    then
	echo cache_pool
	echo ssd" "`hostname -i` >> $ROLES_FILE
	return
    fi

    NH=`cat $ROLES_FILE | grep hdd | wc -l`
    if [ $NH -lt $NUM_HDD ]
    then
	echo storage_pool
	echo hdd" "`hostname -i` >> $ROLES_FILE
	return
    fi

    #NT=`cat $ROLES_FILE | grep td | wc -l`
    #if [ $NT -lt $NUM_TD ]
    #then
	#echo archive_pool
	#echo td"  "`hostname -i` >> $ROLES_FILE
	#return
    #fi
}

get_ip_addr()
{
    IP_ADDR=`cat $IP_ADDR_FILE`
    echo `expr $IP_ADDR + 1` > $IP_ADDR_FILE
    echo "192.168.0."$IP_ADDR
}

wait()
{
    while true
    do
	if [ ! -e $LOCK_FILE ]
	then
	    echo $$ >$LOCK_FILE
	    break
	fi
	sleep 1
    done
}

while true
do
    wait
    if [ $$ = `cat $LOCK_FILE` ]
    then
	break
    fi
done

if [ ! -e $ROLES_FILE ]
then
    touch $ROLES_FILE
fi

if [ ! -e $IP_ADDR_FILE ]
then
    echo "10" > $IP_ADDR_FILE
fi

rm -f $LOCK_FILE
