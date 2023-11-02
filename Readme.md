# Datacenter Networking

Projects on datacenter networking done as part of UW-Madison CS740 course. 

`TCP-SW-DPDK/` : Implements TCP Sliding Window Algorithm in DPDK.<br>
`CONGA-Htsim/` : Implements CONGA congestion control for a DC network in Htsim.<br><br>
`Project/` : Final project on "performance of endhost remote storage and network 
stack in contention".<br>
* Used Fio and SPDK with NVMe-oF to benchmark remote storage traffic
* Used iperf and kernel network optimizations such as GRO, TSO, aRFS to benchmark network
* Used sysstat to measure system performance.   
