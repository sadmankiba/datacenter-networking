#!/bin/bash

echo "check-certificate = no" > ~/.wgetrc

sudo apt update --allow-unauthenticated --allow-insecure-repositories

uname -a

cd /usr/src
scp sadman@192.168.30.2:/home/sadman/bf-sde-zips/linux-4.19.81.tar.gz ./
tar -xzf linux-4.19.81.tar.gz
ln -s /usr/src/linux-4.19.81 /lib/modules/4.19.81-OpenNetworkLinux/build

cd /root/
scp sadman@192.168.30.2:/home/sadman/bf-sde-zips/bf-platforms-netberg-7xx-bsp-9.9.0-202210.tgz ./bf-netberg-bsp-9.9.0.tgz

cd /root/bf-sde-9.9.0/p4studio
bash install-p4studio-dependencies.sh
./p4studio dependencies install 

scp sadman@192.168.30.2:/home/sadman/bf-sde-zips/netberg-asic.yaml /root/bf-sde-9.9.0/p4studio/profiles/
./p4studio profile apply profiles/netberg-asic.yaml





