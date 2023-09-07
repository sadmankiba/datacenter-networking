# Switch Access

This guide describes how to connect to a network switch with Tofino chip. The switch can be accessed through two ports: serial port and management port. Generally, management port is connected through ethernet link to `enoN` interface of a machine and serial port is connected to `/dev/ttySN` port or `dev/ttyUSBN` port of a machine. 

## Connecting through Serial Port

Connect to the switch through serial port using a serial communication tool such as `minicom`.
```
sudo minicom -s
```

Use configurations: 
  - Hardware flow control: No
  - Software flow control: No
  - BPS/parity/bits:
    - 115200 8N1 for Aurora 710
    - 9600 8N1 for BF Wedge-100B 

If you need to use `DEL`, update keyboard settings for backspace key in `minicom` to send `DEL` instead of `BS`.

## Connecting through Management Port

If switch CPU has an OS installed and IP address set up, you can simply SSH into the switch. create a `user/password` after accessing through serial port. It can be used to SSH through management port.  After you SSH into switch, you can change to `root`.

Default user/password
- SONiC in Aurora710: admin/YourPaSsWoRd, 
- ONL: root/onl (may not be used for SSH)

```sh
# SONiC
user@mgmt-server:~$ ssh <user>@192.168.30.3
admin@sonic:~$ sudo su

# ONL
user@mgmt-server:~$ ssh <user>@192.168.30.3 
Password: 
$ su root
Password: # onl
root@localhost:/# cd /root/bf-sde-9.9.0
root@localhost:/root/bf-sde-9.9.0#
```

## Current Information

Aurora 710
- ASIC - Tofino 1
- OS : Distribution - ONL 4.19 Debian 9-based, installation date - March 2023, BF SDE - 9.9.0.

BF Wedge 100B
- ASIC - Tofino 1
- OS : ONL 4.14.151 Debian 9-based, BF SDE - 9.9.0