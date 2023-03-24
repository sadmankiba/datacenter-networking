# CONGA in Htsim

Run<br>
```sh
make
./htsim --expt=1
```

Implementation walkthrough
- First performed congestion information forwarding and receiving in a very simple setup - 1 core router and 2 leaf switches / ToRs.

Added features to implement CONGA in Htsim-
- Flags in `Packet` class
```
class Packet {
    enum Flags {
        ...,
        ACK,
        PASSED_CORE,
    }
}
```
- `class ToR` and `class CoreRouter` inherit `class PacketSink`. A `ToR` object acts as src or dst ToR based on packet flag `PASSED_CORE`.

```
class ToR: PacketSink 
    receivePacket(pkt)
        if packet.flags.PASSED_CORE = true:
            asSrcToR()
        else asDstToR()

    asSrcToR() {
        if pkt is not ack:
            pkt.LBTag = uplink_port
            pkt.CE = 0
        else:
            pkt.vxlan.src = idToR
            pkt.vxlan.dst = dstToRId
            pkt.LBTag = nxtLBTag(dstToRId)
            pkt.CE = CongFromLeaf[dstToRId][pkt.LBTag]
    }
    asDstToR() {
        srcToRId = vxlan_ToR(pkt.vxlan_src)
        if not pkt.is_ack():
            CongFromLeaf[srcToRId][pkt.LBTag] = pkt.CE
        else:
            CongToLeaf[srcToRId][pkt.LBTag] = pkt.CE
    }

    integer dstToRId
    integer idToR

class CoreRouter: PacketSink 
    void receivePacket(pkt) 
        if not pkt.is_ack():
            if link_DRE_congestion[egress_port] > pkt.CE:
                pkt.CE = link_DRE_congestion[egress_port]
        
        pkt.flags.PASSED_CORE = true
```
- Add ToR and core in route
```
...
route.push_back(pServerToR)
route.push_back(ToR)
route.push_back(qToRCore)
route.push_back(pToRCore)
route.push_back(CoreRouter)
route.push_back(qCoreToR)
...
```

- Measure congestion at intermediate switches (PacketSinks) for each egress port.

- Use a congestion matrix to calculate nexthop/path from a switch. Delay/update route for a packet.
- Return congestion feedback from destination ToR to source ToR.

Htsim modifications
- Log records in txt format
- Allow traffic and tcp logging from FlowGenerator class.

Notes-
- Node in Htsim means endhost.