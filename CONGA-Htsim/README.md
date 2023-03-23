# CONGA in Htsim

Run<br>
```sh
make
./htsim --expt=1
```

Implementation walkthrough
- First performed congestion information forwarding and receiving in a very simple setup - 1 core router and 2 leaf switches / ToRs.

Added features to implement CONGA in Htsim-
- Measure congestion at intermediate switches (PacketSinks) for each egress port.
- Add tunnel header fields in packet.
- Use a congestion matrix to calculate nexthop/path from a switch. Delay/update route for a packet.
- Return congestion feedback from destination ToR to source ToR.

Htsim modifications
- Log records in txt format
- Allow traffic and tcp logging from FlowGenerator class.

Notes-
- Node in Htsim means endhost.