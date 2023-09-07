#!/bin/bash

ip route add default via 192.168.30.2
echo 'nameserver 8.8.8.8' >> /etc/resolv.conf

scp sadman@192.168.30.2:/home/sadman/bf-sde-zips/bf-sde-9.9.0.tgz ./

tar -xzf bf-sde-9.9.0.tgz

cd bf-sde-9.9.0


