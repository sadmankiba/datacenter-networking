#ifndef CONGA_H
#define CONGA_H

#include "network.h"
#include "queue.h"
#include <vector>

using namespace std;

class ToR: public PacketSink {
public:
    ToR(): dstToRId(0) {
        idTor = NUMTOR;
        ToR::NUMTOR++;
        for(int i = 0; i < 256; i++)
            nxtLbTag.push_back(0);
        

    }
    void receivePacket(Packet &pkt) {
        if (packet.getFlag(Packet::PASSED_CORE) == 0) {
            asSrcToR(pkt)
        } else asDstToR(pkt)
    }

    void asSrcToR(Packet &pkt) {
        if (pkt.getFlag(Packet::ACK) == 0) {
            pkt.vxlan.lbtag = 0; // uplink_port, set from route
            pkt.vxlan.ce = 0; 
        } else {
            uint8_t dstToRId = ((ToR) *(pkt.getRoute())[-3]).idTor;
            pkt.vxlan.src = idTor;
            pkt.vxlan.dst = dstToRId;
            pkt.vxlan.lbtag = nxtLbTag[dstToRId];
            pkt.vxlan.ce = CongFromLeaf[dstToRId][pkt.vxlan.lbtag];
        }
    }

    void asDstToR(Packet &pkt) {
        if(pkt.getFlag(Packet::ACK) == 0) {
            CongFromLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
        } else {
            CongToLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
        }
    }

    uint8_t idTor;
private:
    vector<uint8_t> nxtLbTag;
    vector<vector<uint8_t>> CongFromLeaf{0};
    vector<vector<uint8_t>> CongToLeaf{0};
    static uint8_t NUMTOR;
}

class CoreQueue: public Queue {
public:
    void receivePacket(Packet &pkt) {
        uint8_t egPort = ( (ToR) *(pkt.getRoute())[pkt.getNextHop() + 1]).idToR;
            
        if (pkt.getFlag(Packet::ACK) == 0) {
            if (regCong[egPort] > pkt.vxlan.ce) {
                pkt.vxlan.ce = (uint8_t) regCong[egPort] * 8;
            }
        }
        updateRegCong(egPort);
    }
private:
    void updateRegCong(uint8_t egPort) {
        if (regCong[egPort] == 0) 
            regCong[egPort] = _queuesize * 1.0 / _maxsize;
        else {
            regCong[egPort] = regCong[egPort] * 7 / 8 + (_queuesize * 1.0 / _maxsize) * 1 / 8;
        }
    }
    vector<double> regCong(256);
}

#endif 