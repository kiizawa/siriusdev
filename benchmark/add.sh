#!/bin/sh

sum=0
while read line
do
    sum=`expr $sum + $line`
    echo $sum
done < $1
