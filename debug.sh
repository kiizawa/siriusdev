#!/bin/bash

OBJ=$1

for i in `seq 13 24`
do
    node=192.168.0.$i
    ssh $node hostname
    ssh $node cat /dev/shm/osd*log | grep -e get_onode -e osd_op_reply | grep $OBJ
done
