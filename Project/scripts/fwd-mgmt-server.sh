sudo sysctl net.ipv4.conf.all.forwarding=1
sudo iptables -P FORWARD ACCEPT
sudo iptables -t nat -I POSTROUTING 1 -s 192.168.30.0/24 -j MASQUERADE
