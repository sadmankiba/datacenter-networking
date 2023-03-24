#include "conga.h"

uint8_t ToR::TOR_ADDED = 0;

ToR::ToR(): Logged("ToR") {
    idTor = TOR_ADDED;
    ToR::TOR_ADDED++;
    for(int i = 0; i < N_TOR; i++)
        nxtLbTag.push_back(0);
    
    vector<uint8_t> v;
    for (uint8_t i = 0; i < ToR::N_TOR; i++) {
        v.clear();
        for (uint8_t j = 0; j < CoreQueue::N_CORE; j++) {
            v.push_back(0); 
        }
        CongFromLeaf[i] = v;
        CongToLeaf[i] = v;
    }
}
void ToR::receivePacket(Packet &pkt) {
    if (pkt.getFlag(Packet::PASSED_CORE) == 0) {
        asSrcToR(pkt);
    } else asDstToR(pkt);
}

void ToR::asSrcToR(Packet &pkt) {
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_TOR_SRC);
    /* pkt.route = ... - dstToR - pServerToR - qServerToR - tcpSink */
    uint8_t dstToRId = ((ToR *) ((*(pkt.getRoute()))[pkt.getRoute()->size() - 4]))->idTor;
    if (pkt.getFlag(Packet::ACK) == 0) {
        pkt.vxlan.src = idTor;
        pkt.vxlan.dst = dstToRId;
        pkt.vxlan.lbtag = minCongestedCore(dstToRId);
        pkt.vxlan.ce = 0; 
    } else {
        pkt.vxlan.src = idTor;
        pkt.vxlan.dst = dstToRId;
        pkt.vxlan.lbtag = nxtLbTag[dstToRId];
        pkt.vxlan.ce = CongFromLeaf[dstToRId][pkt.vxlan.lbtag];
        updateNxtLbTag(dstToRId);
    }
}

void ToR::asDstToR(Packet &pkt) {
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_TOR_DST);
    if(pkt.getFlag(Packet::ACK) == 0) {
        CongFromLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
    } else {
        CongToLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
    }
}

uint8_t ToR::minCongestedCore(uint8_t dstToR) {
    uint8_t minCore = 0;
    uint8_t minCg = CongToLeaf[dstToR][0];
    for (uint8_t i = 1; i < CoreQueue::N_CORE; i++) {
        if (CongToLeaf[dstToR][i] < minCg) {
            minCore = i;
            minCg = CongToLeaf[dstToR][i];
        } 
    }
    return minCore;
}

void ToR::updateNxtLbTag(uint8_t dstToR) {
    nxtLbTag[dstToR] = (nxtLbTag[dstToR] + 1) % CoreQueue::N_CORE;
}