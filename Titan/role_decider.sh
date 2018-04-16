#!/bin/sh

set -x

CEPH_SCRIPTS_DIR=$PROJWORK/csc143/$USER/ceph/scripts
LOCK_FILE=$CEPH_SCRIPTS_DIR/lock
ROLES_FILE=$CEPH_SCRIPTS_DIR/roles
IP_ADDR_FILE=$CEPH_SCRIPTS_DIR/ip_addr

source $CEPH_SCRIPTS_DIR/setting

is_mon()
{
    set +e
    MON_RUNNING=`cat $ROLES_FILE | grep mon`
    set -e
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
    set +e
    NS=`cat $ROLES_FILE | grep ssd | wc -l`
    set -e
    if [ $NS -lt $NUM_SSD_NODES ]
    then
	echo cache_pool
	echo ssd" "`hostname -i` >> $ROLES_FILE
	return
    fi

    set +e
    NH=`cat $ROLES_FILE | grep hdd | wc -l`
    set -e
    if [ $NH -lt $NUM_HDD_NODES ]
    then
	echo storage_pool
	echo hdd" "`hostname -i` >> $ROLES_FILE
	return
    fi

    #NT=`cat $ROLES_FILE | grep td | wc -l`
    #if [ $NT -lt $NUM_TD_NODES ]
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
