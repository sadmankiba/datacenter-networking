# CONGA in Htsim

Run<br>
```sh
make
./htsim --expt=1
```

Implementation walkthrough
- First performed congestion information forwarding and receiving in a very simple setup - 1 core router and 2 leaf switches / ToRs.

Added features to implement CONGA in Htsim-
- `class SrCToR`, `class CoreRouter` and `class DstToR` inherit `class PacketSink`. 

```
class ToR: public PacketSink {
    uint8_t dest_ToR();
    uint8_t nxtLBTag();

    vector<uint8_t> vxlan_ip;
    vector<vector<uint8_t>> LBTagPref;
}

class SrcToR: public ToR {
    void receivePacket() {
        if not pkt.is_ack():
            pkt.LBTag = uplink_port
            pkt.CE = 0
        else:
            dstToRId = dest_ToR(pkt.dst_ip)
            pkt.vxlan_src = vxlan_ip[ToRId]
            pkt.vxlan_dst = vxlan_ip[dstToRId]
            pkt.LBTag = nxtLBTag(dstToRId)
            pkt.CE = CongFromLeaf[dstToRId][pkt.LBTag]
    }
}

class DstToR: public ToR {
    void receivePacket() {
        srcToRId = vxlan_ToR(pkt.vxlan_src)
        if not pkt.is_ack():
            CongFromLeaf[srcToRId][pkt.LBTag] = pkt.CE
        else:
            CongToLeaf[srcToRId][pkt.LBTag] = pkt.CE
    }
}

class CoreRouter: public PacketSink {
    void receivePacket() {
        if not pkt.is_ack():
            if link_DRE_congestion[egress_port] > pkt.CE:
                pkt.CE = link_DRE_congestion[egress_port]
    }
}
```

- Measure congestion at intermediate switches (PacketSinks) for each egress port.
- Add tunnel header fields in packet.
- Use a congestion matrix to calculate nexthop/path from a switch. Delay/update route for a packet.
- Return congestion feedback from destination ToR to source ToR.

Htsim modifications
- Log records in txt format
- Allow traffic and tcp logging from FlowGenerator class.

Notes-
- Node in Htsim means endhost.