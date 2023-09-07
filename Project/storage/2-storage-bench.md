# Storage Benchmarking

## Fio
The `Fio` application can measure latency and bandwidth of IO workloads with different IO engines, block size and other IO configurations. The filename can be a regular file or a volume in local drive or exported remote drive. The `bench-fio.sh` script can `Fio` workloads with different configuration (e.g. read/write, 4k/128k block etc.).<br>
```sh
./bench-fio.sh -e psync -f /dev/nvme0n1 -i randread
# Multiple volumes
./bench-fio.sh -e libaio -f "/dev/nvme2n4 /dev/nvme2n3 /dev/nvme2n2 /dev/nvme2n1" -b 128k -q 4 -i randread -s 10G -t 10
``` 


`Fio` supports xNVMe IO engine that can interact with NVMe device from user space. To use xNVMe engine IO engine, use fio version >= 3.33. You can download the latest release from Github repository of `Fio`. The userspace library of xNVMe is re-targetable and can use Linux NVMe drivers such as `libaio`, `IOCTLs` and `io_uring`, SPDK NVMe driver or custom NVMe driver. The [`xnvme-bench-exmp.fio`](../scripts/xnvme-bench-exmp.fio) job file uses Linux `libaio` driver for xNVMe engine. Run this job as-<br>
`SIZE=2G FILENAME=/dev/nvme0n1p3 fio xnvme-bench-exmp.fio`

To use SPDK driver for xNVMe IO engine in Fio, 
1. First make sure no filesystem is mounted on the NVMe device. Check with `lsblk` command.
2. Follow [userspace section](https://xnvme.io/docs/latest/getting_started/index.html#user-space) in xNVMe getting started guide to bind xNVMe driver with NVMe device. You will need to set IOMMU on in BIOS menu and in GRUB scripts in `/etc/grub.d` - if IOMMU is set to off in these. To set in GRUB script, in the `linux` line of grub scripts, if `intel_iommu=off` or `amd_iommu=off`, set these to `on` and then run `update-grub`. After enabling IOMMU, the `dmesg | grep IOMMU` may not show any output. But, `find /sys/kernel/iommu_groups -type l` should show some devices. Bind xNVMe driver, which is built on top of SPDK driver, to NVMe device with-<br>
```sh
xnvme-driver # To bind Linux driver back, run 'xnvme-driver reset'
```

3. In xNVMe source directory, run an example to test if xNVMe driver has bind properly,
```sh
cd xnvme
gcc examples/xnvme_hello.c $(pkg-config --libs xnvme) -o hello
xnvme enum
./hello hw 0000\:22\:00.0 # Here, device uri is 0000:22:00.0 
```
4. Test xNVMe driver (SPDK driver) for Fio with [spdk-xnvme-exmp.fio](../scripts/spdk-xnvme-exmp.fio). This script only works with local PCIe-connected NVMe devices.
`FILENAME=0000\\:22\\:00.0 fio spdk-xnvme-exmp.fio`

A Fio maintainer has written blog posts on using xnvme engine with Fio - [xnvme-fio-1](https://github.com/vincentkfu/fio-blog/wiki/xNVMe-ioengine-Part-1), [xnvme-fio-2](https://github.com/vincentkfu/fio-blog/wiki/xNVMe-ioengine-Part-2). 

## SPDK 

SPDK can be installed separately to use the tools that come with SPDK. SPDK has a fio plugin and a `perf` application similar to `Fio`. Documentation of the SPDK `perf` tool can be found in [SPDK Nvme Driver Page](https://spdk.io/doc/nvme.html) and source code is in `examples/nvme/perf/` directory. After installing SPDK with `make install`, the `perf` tool is available as `spdk_nvme_perf` command. The [spdk-perf.sh](../scripts/spdk-perf.sh) script uses SPDK `perf` tool to measure performance of local and remote NVMe device.

- First, make sure no filesystem is mounted on the NVMe device. You can check this with `lsblk` command. Resources for moving filesystem to another storage drive- 
  1. How to move Linux partition to another drive quickly, Medium blog
  2. How to migrate the root filesystem to a new disk, SUSE Support 

- Now, unbind kernel driver and bind SPDK driver- 
```sh
cd spdk   # spdk repository
# This will bind to local SSD, but won't bind to remote SSD
sudo ./scripts/setup.sh 
```
- Then, run `spdk-perf.sh` script<br>
```
cd scripts
./spdk-perf.sh -t TCP -a 192.168.50.7
```

If it fails, you will see `connect() failed` errors. If it runs fine, you might see some messages like `TELEMETRY: No legacy callbacks, legacy socket not created`. 

- To rebind kernel driver,
```sh
sudo ./scripts/setup.sh reset
```

To see RPC options, run
```
./scripts/rpc.py <RPC name> -h
```


### Troubleshooting 

_Error: Read Only Filesystem after binding SPDK driver_<br>
Solution: This is likely because the root filesystem was mounted on the NVMe device. As said above, you cannot have filesystem on the NVMe device when binding SPDK. If you reboot the machine, this error will be gone. The `reboot` command from command line might fail to shutdown the machine because of the read-only file system. In that case, you will need to physically reboot the machine with the power button. 

## Fungible 

Doc - https://docs.google.com/document/d/1U_eza1tSzCa8S65sQq-pmde5ZSXCuoKXcGNjJJgf-SQ/edit?usp=sharing 