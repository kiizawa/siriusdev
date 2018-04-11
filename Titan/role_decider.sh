#!/bin/sh

set -x

NUM_SSD=4
NUM_HDD=2
NUM_TD=1

LOCK_FILE=/tmp/share/lock
ROLES_FILE=/tmp/share/roles

is_mon()
{
    MON_RUNNING=`cat $ROLES_FILE | grep mon`
    if [ -z $MON_RUNNING ]
    then
	echo 1
	echo mon >> $ROLES_FILE
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
	echo ssd >> $ROLES_FILE
	return
    fi

    NH=`cat $ROLES_FILE | grep hdd | wc -l`
    if [ $NH -lt $NUM_HDD ]
    then
	echo storage_pool
	echo hdd >> $ROLES_FILE
	return
    fi

    NT=`cat $ROLES_FILE | grep td | wc -l`
    if [ $NT -lt $NUM_TD ]
    then
	echo td
	echo td >> $ROLES_FILE
	return
    fi
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

RUN_MON=`is_mon`
POOL=`get_pool`

rm lock
