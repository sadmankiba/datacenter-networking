#include "network.h"
#include <vector>

using namespace std;

class ToR: public PacketSink {
public:
    ToR() {
        idTor = NUMTOR;
        ToR::NUMTOR++;
    }
    uint8_t dest_ToR();

    vector<uint8_t> vxlan_ip;
    vector<vector<uint8_t>> LBTagPref;

    uint8_t idTor;
private:
    static uint8_t NUMTOR;
}

class SrcToR: public ToR {
public:
    SrcToR(): dstToRId(0) {
        nxtLbTag.reserve(256);
    }
    void receivePacket(Packet &pkt) {
        if (pkt.getFlag(Packet::ACK) == 0) {
            pkt.vxlan.lbtag = 0; // uplink_port, set from route
            pkt.vxlan.ce = 0; 
        } else {
            pkt.vxlan.src = idTor;
            pkt.vxlan.dst = dstToRId;
            pkt.vxlan.lbtag = nxtLbTag[dstToRId];
            pkt.CE = CongFromLeaf[dstToRId][pkt.vxlan.lbtag];
        }
    }

    uint8_t dstToRId; // set from route
private:
    vector<uint8_t> nxtLbTag;
    vector<vector<uint8_t>> CongFromLeaf;
}

class DstToR: public ToR {
    void receivePacket() {
        uint8_t srcToRId = pkt.vxlan.src;
        if(pkt.getFlag(Packet::ACK)) {
            CongFromLeaf[srcToRId][pkt.vxlan.lbtag] = pkt.CE;
        } else {
            CongToLeaf[srcToRId][pkt.vxlan.lbtag] = pkt.CE;
        }
    }
private:
    vector<vector<uint8_t>> CongFromLeaf;
    vector<vector<uint8_t>> CongToLeaf;
}

class CoreRouter: public PacketSink {
    void receivePacket() {
        if (pkt.getFlag(Packet::ACK)) {
            if (link_DRE_congestion[egress_port] > pkt.CE) {
                pkt.CE = link_DRE_congestion[egress_port]
            }
        }
    }
}