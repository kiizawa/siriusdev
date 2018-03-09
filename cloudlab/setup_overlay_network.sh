#!/bin/sh

if [ `hostname` = "node-1" ]
then
    cd /tmp
    curl -L  https://github.com/coreos/etcd/releases/download/v2.3.4/etcd-v2.3.4-linux-amd64.tar.gz -o etcd-v2.3.4-\linux-amd64.tar.gz
    tar xzvf etcd-v2.3.4-linux-amd64.tar.gz
    sudo chmod a+w etcd-v2.3.4-linux-amd64
    cd etcd-v2.3.4-linux-amd64
    ./etcd -listen-client-urls "http://10.10.1.2:2379" -advertise-client-urls "http://10.10.1.2:2379" &
fi

IFs="eth0 eth1 eth2 eth3"

for IF in $IFs
do
    IP=`echo $(ip --oneline --family inet address show dev $IF) | cut -d ' ' -f 4 | cut -d '.' -f 1-3`
    if [ -z "$IP" ]
    then
	continue
    fi
    if [ $IP = "10.10.1" ]
    then
	CEPH_IF=$IF
    fi
done

sudo env CEPH_IF=$CEPH_IF sh -c 'echo DOCKER_OPTS=\"--cluster-store=etcd://10.10.1.2:2379 --cluster-advertise=$CEPH_IF:2376\" >> /etc/default/docker'
sudo service docker restart

if [ `hostname` = "node-1" ]
then
    docker network create -d overlay --subnet=192.168.0.0/16 cephnet
fi
