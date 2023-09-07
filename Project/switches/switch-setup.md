# Switch Setup

This guide describes how to set up IP connection, install OS and install BF SDE in a network switch.

## IP Connection Setup

The term `mgmt-server` is used to refer to the machine that is connected to management port. To connect through management port, you'll need to set up an IP connection between switch main CPU and `mgmt-server`. Once you set up the IP connection, you can SSH into the switch from `mgmt-server`. For detailed instructions, see section 2.1 of BF switch user guide(UG)[1].

### Assign IP address 
Assign IP address as follows:
1. Add a private IP to the `enoN` interface of `mgmt-server` that is connected to switch.<br>  
`user@mgmt-server: $ ip addr add 192.168.30.2/24 dev eno2`

2. Access switch through serial port. BF Wedge 100B switch has two CPUs - BMC CPU and main CPU. To access main CPU, you will need to run `sol.sh` from BMC CPU. For troubleshooting main CPU connection, see next section.

3. Add a private IP to `eth0` (or, `ma1`) interface of switch main CPU. This can be done in ONIE prompt or in an installed OS.
```
ONIE:~ # ip addr del <existing-address> dev eth0
ONIE:~ # ip addr add 192.168.30.3/24 dev eth0
```

4. Check if you can `ping` `mgmt-server` from switch main CPU and vice versa.

5. The default user is `root`. For some reason, switch main CPU may not allow setting up SSH connection using default `root` user. Then, you will need to create a new user and password in switch main CPU. After that use those credentials to SSH into switch main CPU.

### Switch Main CPU Troubleshooting
- If the main CPU does not respond, follow the workaround in section 9.1.1 of switch UG[1] to reboot main CPU. 
- If the OS in main CPU does not function properly, you can reinstall an OS. See the section below for installating OS. 
- `reboot` and then press ESC/DEL keys to go to the boot menu. Then, select ONIE>Rescue which will take you to ONIE prompt.
```
ONIE:~ #
```

From this prompt, you can browse filesystem, configure network interfaces and install OS in main CPU. If ONIE Prompt keeps checking for DHCP on eth0, run 
```sh
ONIE~# onie-discovery-stop
# Or run `ps` to find `pid` of process `/bin/discover` and kill that process
kill -9 PID
```

### Connecting Switch Main CPU to Internet

To install BF SDE and any other packages in switch main CPU, you'll need to connect it to Internet. 
1. Connect switch to Internet by forwarding traffic through `mgmt-server`. You can do this by adding a NAT rule with `iptables` command in `mgmt-server`.

```
user@mgmt-server:~$ sudo sysctl net.ipv4.conf.all.forwarding=1
user@mgmt-server:~$ sudo iptables -P FORWARD ACCEPT
user@mgmt-server:~$ sudo iptables -t nat -I POSTROUTING 1 -s 192.168.30.0/24 -j MASQUERADE
```

2. Then, in switch main CPU, route all traffic through `mgmt-server` and add DNS server.

```
root@localhost:~# ip route add default via 192.168.30.2 # mgmt-server ip
root@localhost:~# vim /etc/resolv.conf # add a line : nameserver 8.8.8.8
```

## Installing ONL OS in main CPU 

If switch main CPU is not functioning or has an outdated OS, you may need to install OS in main CPU. Generally, regular OS such as Debian and Ubuntu and network OS, such as [OpenNetworkLinux](https://github.com/opennetworklinux/ONL/tree/master/docs), can be installed in the switch. 

### BF Wedge 100B

#### Obtaining ONL installer
Download an installer from [Open Network Linux Download site](http://opennetlinux.org/binaries/) or build ONL from source code on a Debian machine. 

To run P4 studio SDE in BF Wedge 100B hardware you'll need some kernel build files. For this, ONL can be built from source code on a Debian 9 machine (commit checksum: dd61cc1792290a459c394c5b7bdf4d4a27b6f651). Copy `$ONL/REPO/stretch/extracts/onl-kernel-4.14-lts-x86-64-all_amd64/usr/share/onl/packages/amd64/onl-kernel-4.14-lts-x86-64-all/mbuilds/` into `/lib/modules/<ONL-version>/build/` directory in switch main CPU. This solution was suggested in Zhang's switch user manual. The ONL installer and kernel build artifacts can be found [here](https://drive.google.com/drive/folders/1Uf6NZ2cZ-XyMUOGuJaggezbxYUNl3hh8?usp=sharing). 

#### OS Installation procedure 

Go to ONIE prompt as described in troubleshooting section above. Then, use `scp` to copy installer from the host connected to mgmt port. For details, see section 7.2 of BF switch UG[1]. Now, install:
`ONIE:~ $ onie-nos-install <ONL-INSTALLER>`

If the installer does not suit this switch, installation will fail and after an automatic reboot, you'll be back to ONIE prompt. If installation succeeds, after an automatic reboot, you'll see `Open Network Linux` in Grub menu and OS will finish installation with following output:

```sh
Stopping watchdog keepalive daemon....
Starting watchdog daemon....

Open Network Linux OS ONL-master, 2019-07-23.01:53-01d1564

localhost login: 
```

### Aurora 710

Follow [Netberg build guide](./netberg-bf-sde-onl-build-steps-9.7.0.txt).  The installer, bsp and kernel source can be downloaded from Netberg FTP server (`domain: ftp.netbergtw.com, user: ftpuser, pass: Harn-FIVE-Quer`). 

ONL installer from ONL website, Ubuntu and bsp from intel R&D portal can not run BF SDE properly on Aurora 710. 

## Installing P4 Studio SDE in Switch Main CPU

Download BF SDE zip file and installation guide from Intel resource center. Move BF-SDE install zip file to switch and follow section 3 of switch UG[1]. Some commands in that guide are outdated. So, also see P4 studio SDE installation guide[2]. In summary, do the following:
- Extract installation file.
- Install dependencies.
- Install SDE with `p4studio` program.

To run P4 programs on Tofino model, you can use the `all-tofino.yaml` profile.

To run P4 programs on Tofino ASIC, you'll need to do the following:
1. You'll need to have Linux kernel files in `/lib/modules/<ONL-version>/build` directory of switch main CPU. For Aurora 710, download from FTP server.
2. Download P4 studio BSP and copy it to switch main CPU. For BF Wedge 100B, use 9.9.0 Reference BSP from Intel resource center. For Aurora 710, use the BSP from the Netberg FTP server.  
3. Use the following profile instead of `all-tofino.yaml` to build and install P4 studio SDE. For Aurora 710, check [Netberg build guide](./netberg-bf-sde-onl-build-steps-9.7.0.txt).

```yaml
global-options: 
  asic: true  # builds bf_kdrv.ko driver module
features:
  bf-diags:
    thrift-diags: true
  bf-platforms:
    # builds SDE_INSTALL/lib/libpltfm_mgr.so, that allows bf_switchd to use Tofino ASIC instead of Tofino model
    bsp-path: /root/bf-reference-bsp-9.9.0.tgz        
  drivers:
    bfrt: true
    bfrt-generic-flags: true
    grpc: true
    p4rt: true
    thrift-driver: true
  p4-examples:
    - p4-14-programs
    - p4-16-programs
  switch:
    profile: x1_tofino
    sai: true
    thrift-switch: true
architectures:
  - tofino
```

### P4 Studio SDE Installation Issues 

**/lib/modules/kernel-version/build not exist**
Copy Linux kernel build files into this directory.


**Certificates of `kernel.org` are expired**
While installing dependencies for BF SDE in main CPU, if it shows that certificates of `kernel.org` are expired, do the following:<br>
`root@localhost:~# echo "check-certificate = no" > ~/.wgetrc`

## Resources

Aurora 710 Switch
- [Netberg Aurora 710 installation guide](https://netbergtw.com/wp-content/uploads/Files/aurora_710_manual_en.pdf) <br>
- [Netberg Support email](mailto: support@netbergtw.com)

Tofino-based Switch
- [Wedge 100B Switch User Guide](./10k-UG2-018ea_Wedge-UG.pdf). 

OS and BF-SDE Installation
- P4 studio SDE 9.8.0 installation guide, February 2022. (downloaded from Intel resource & documentation center)
- [Zhang's Wedge 100B Switch User Manual](https://gitlab.tongyuejun.cn/zhangjx/p4_doc/-/blob/main/Wedge100BF_User_Manual.org) - Mostly similar to switch UG[1], with additional tips. 
- [ONL Google Group](https://groups.google.com/g/opennetworklinux)