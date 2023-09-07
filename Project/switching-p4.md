# Switching with P4 in BF-SDE

## General

The source code for `switch.p4` is located in `bf-sde-a.b.c/pkgsrc/switch-p4-16`. For using bf-switch CLI in `bfshell`, check BF Switch CLI guide. The commands can be autocompleted by pressing TAB after typing first one or two characters. You can also use ? anytime you need help with the rest of the command.

Port `handle` is the object id(oid) of a port in `show port all` command.

To keep running `bf_switchd` even after SSH connection is closed, use `-r` option to redirect output when running `run_switchd.sh`. Later, run `run_bfshell.sh` to access CLI.
```sh
root@localhost:/bf-sde-9.9.0# ./run_switchd.sh -p switch -r switchd.log
root@localhost:/bf-sde-9.9.0# ./run_bfshell.sh
```

To stop `switchd`, run `killall -r bf_switchd`.


## `switch.p4` program

`switch.p4` is a P4 program shipped with BF SDE, that provides basic features of a network switch. Supported features include L2 switching, L3 routing and tunneling. It also provides API and CLI to manage network objects, such as vlan and ports, in a switch. `switch.p4` program contains different profiles (e.g. x1_tofino, y2_tofino2) to provide slightly different features. It's source code is located in `<SDE>/pkgsrc/switch-p4-16`.

### Schema
The BF Switch exposes a set of CRUD (Create, Read, Update and Delete) APIs to uniformly manage a set of user objects such as vlan, port, etc. defined in an object schema. The objects are internally mapped to various P4 tables. Switch API object types are schema driven and are auto generated from a schema defined in a JSON file.

### Running
Running `switch.p4` with `bf_switchd` will provide a `bfshell> ` prompt. You can then follow Switch CLI [2] to manage network resources. 

### Configuring L2 switching with switch.p4 program

After installing BF SDE and loading kernel module, switch.p4 can be used to enable L2 switching. 

Run switch.p4, get a `bfshell`, add desired ports to a vlan and set learning true. This will enable the switch to learn MAC table and perform switching. See [1]. 
The commands from [1] are copied below for ease of use-

```sh
bfshell> bf_switch
bf_switch:0> add port type NORMAL port_id 9 speed 100G fec_type NONE autoneg DISABLED admin_state true port_vlan_id 1 learning true 
bf_switch:0> show port all
...
6       NORMAL  9         260       100G     UP          True         DISABLED  NONE      0
bf_switch:0> show vlan all
bf_switch:0> add vlan_member member_handle port 6 vlan_handle vlan 1 tagging_mode UNTAGGED # port num 6 is oid from 'show port all' command 
bf_switch:0> show vlan_member all
```

The `port_vlan_id` attribute of a port is the VLAN id, which is different from VLAN handle (oid). Difference from [1]:
For BF SDE 9.9.0,
- `port_id` in `add port` command starts from 0. There are 4 virtual ports for each physical port. So, port 3/1 has `port_id` 9.
- `member_handle` instead of `port_lag_handle` in `add vlan_member`.

### Port operations 

**Reverting the port added**<br>
Delete `vlan_member` and `port`.
```
bf_switch:0> del vlan_member handle <oid>
bf_switch:0> del port handle <oid>
```

**Updating an attribute of port**<br>
An example of updating vlan:
```
bf_switch:0> set port handle 8 attribute port_vlan_id 1
```

**Checking number of frames received and transmitted throgh a port**
```sh
bfshell> ucli
bf-sde> pm show  # or pm show -a
# Or
bf_switch:0> show counter port handle 5
```

**Bring down a port**
```sh
bf_switch:0> set port handle 5 attribute admin_state false
```

### Configuring LAG

Use `add lag` and `add lag_member` in BF Switch CLI.
```sh
bf_switch:0> add lag learning true port_vlan_id 1
bf_switch:0> add lag_member lag_handle lag 1 port_handle port 9 
bf_switch:0> add vlan_member member_handle lag 1 vlan_handle vlan 1
```

To use LAG from a server, creating NIC bonding between the interfaces in `balance-rr` (round robin) mode. 

### Configuring MTU for Jumbo frames

Use `rx_mtu` and `tx_mtu` attributes when using `add port`.

## Creating MAC Table manually

If we use a p4 program such as `basic_switching.p4` that does not learn MAC addresses, we will need to add MAC table entries manually. 

Run P4 program.

```sh
./run_switchd.sh -p basic_switching
```

Add two ports (e.g. port 3 and 5)
```
bfshell> ucli
bf-sde> pm
bf-sde.pm> port-add 3/- 100G RS
bf-sde.pm> port-add 5/- 100G RS 
bf-sde.pm> port-enb 3/-
bf-sde.pm> port-enb 5/-
bf-sde.pm> show
-----+----+---+----+-------+----+--+--+---+---+---+--------+----------+----------+
PORT |MAC |D_P|P/PT|SPEED  |FEC |AN|KR|RDY|ADM|OPR|LPBK    |FRAMES RX |FRAMES TX |E
-----+----+---+----+-------+----+--+--+---+---+---+--------+----------+----------+-
3/0  |21/0|144|3/16|100G   | RS |Au|Au|YES|ENB|UP |  NONE  |        24|         0|
5/0  |19/0|160|3/32|100G   | RS |Au|Au|YES|ENB|UP |  NONE  |        18|         0|
```

Here, `ADM` should be `ENB` and `OPR` should be `UP`. If `OPR` is `DWN`, try adding ip addresses to the interfaces in the hosts. Suppose, the host at 3/0 has IP `192.168.70.5` and MAC address `b8:ce:f6:b0:30:fb` and the host at 5/0 has IP `192.168.70.7` and MAC address `b8:ce:f6:b0:31:2b`.

Now, add MAC table entries-
```sh
bf-sde.pm> exit
bfshell> pd-basic-switching device 0
pd-basic-switching:0> pd init 
# 144 and 160 are D_P field in pm.show
pd-basic-switching:0> pd forward add_entry set_egr ethernet_dstAddr b8:ce:f6:b0:30:fb action_egress_spec 144 
pd-basic-switching:0> pd forward add_entry set_egr ethernet_dstAddr b8:ce:f6:b0:31:2b action_egress_spec 160
pd-basic-switching:0> pd forward get_entry entry_hdl 2
match_spec_ethernet_dstAddr: 0xB8CEF6B030FB
action_spec_name: p4_pd_basic_switching_set_egr
action_egress_spec: 144 (0x90)
pd-basic-switching:0> exit
```

Since, `basic_switching.p4` can not perform ARP broadcast, we will manually add ARP cache entries in both hosts. At the host connected to 3/0,
```
sudo arp -i ens3f1 -s 192.168.70.7 b8:ce:f6:b0:31:2b
```
and similarly to the other host. Now, you should be able to `ping` between the two hosts. 

## Miscellaneous
Show transceiver information at a port.
```
bfshell> ucli
bf-sde> qsfp dump-info <PHYSICAL-PORT-NUMBER>
```

In `ucli` shell, change to parent with `..` command.

## Reference
[1] [`switch.p4` ICRP KB](https://community.intel.com/t5/Intel-Connectivity-Research/Configuring-basic-L2-switching-using-switch-p4-data-plane/ta-p/1376460)
[2] BF Switch CLI Guide, Oct 2018. Intel Research and Documentation center.
