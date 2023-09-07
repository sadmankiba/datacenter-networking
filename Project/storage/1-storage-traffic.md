# Storage Traffic

This guide describes how to generate storage traffic over a network. Either RDMA or NVMe over Fabric can be used to generate such traffic. 

## RDMA
To create RDMA traffic: 
- Install Ubuntu packages: `libibverbs-dev`, `libibverbs1`, `rdma-core`, `ibverbs-utils`
- Load kernel modules: `ib_uverbs`, `rdma_ucm`, `rdma_rxe`, `mlx5_core`
- Install [`perftest`](https://github.com/linux-rdma/perftest). Installing from source code is better.

Create RDMA traffic as below:<br>
In one machine, run server. Find rdma interface name with `rdma link` command. Here it is `mlx5_1`. <br>
`ib_write_bw -d mlx5_1 -i 1 -p 9001`

In another machine, run client<br>
`ib_write_bw -n 10000 -F -s 2048 -m 256 -c RC -d mlx5_1 -i 1 -p 9001 <server ip>`

It is recommended to use same version of packages in both machines.


## NVMe over Fabrics (NVMe-oF)

To deploy NVMe-oF, we can either use NVMe over RDMA ([setup guide](https://www.linuxjournal.com/content/data-flash-part-ii-using-nvme-drives-and-creating-nvme-over-fabrics-network)) or NVMe over TCP ([setup guide](https://www.linuxjournal.com/content/data-flash-part-iii-nvme-over-fabrics-using-tcp)). In summary, the setup requires:
- Installing `nvme-cli` and installing RDMA packages.
- Loading NVMe and RDMA kernel modules.
- Exporting a partition or logical volume from the target machine on a network interface.
- Connecting to the exported drive from the host machine.<br> 
The script [`export-nvme-lv.sh`](../scripts/export-nvme-lv.sh) can be run to export a logical volume from the target. 

After connecting to the remote NVMe drive from host, you should see the remote drive in `nvme list` command. Then,
- Make a filesystem on the drive if not already created.<br>
`mkfs -t ext4 /dev/nvme2n1`
- Mount the drive on a directory to read or write files. <br>
`mount /dev/nvme2n1 remote/`

**Using scripts after necessary packages are installed**
```sh
# In target,
cd scripts
./export-nvme-lv.sh -t tcp -a 192.168.50.7

# In host,
cd scripts
./load-nvme-mod.sh -t tcp
./nvme-con.sh -t tcp -m cn -a 192.168.50.7


# To remove export
./rm-export-nvme-lv.sh 
```

Now, any read or write operation to a file in that directory will be done over the network. `Fio` is an application to generation storage traffic from configuration and measure performance. `Fio` can be run on the connected drive without mounting it to a directory. By default, `Fio` creates jobs in the current directory. So, to create storage traffic on the remote drive, either change to `remote/` directory before running `Fio` or use `filename=<remote-device>` (e.g. /dev/nvme2n1). 

In SSD, if an address for a read request does not exist in the flash translation table, the controller does not go to flash channels, rather returns 0. So, before doing a read benchmark, it is better to write whole volume used for the bench. 
```sh
./write-lv-full -d /dev/nvme0n1p
```   

## Troubleshooting

E: Failed to open /dev/nvme-fabrics: No such file or directory
Soln: Check with `lsmod` if nvme-tcp or nvme-rdma modules are loaded. 
