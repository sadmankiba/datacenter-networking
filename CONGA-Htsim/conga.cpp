#include "conga.h"

uint8_t ToR::TOR_ADDED = 0;

ToR::ToR(uint8_t idTor, Logger *logger): Logged("ToR"), idTor(idTor), _logger(logger) {
    
    for(int i = 0; i < N_TOR; i++)
        nxtLbTag.push_back(rand() % CoreQueue::N_CORE);
    
    vector<uint8_t> v;
    for (uint8_t i = 0; i < ToR::N_TOR; i++) {
        v.clear();
        for (uint8_t j = 0; j < CoreQueue::N_CORE; j++) {
            v.push_back(0); 
        }
        CongFromLeaf.push_back(v);
        CongToLeaf.push_back(v);
    }
}
void ToR::receivePacket(Packet &pkt) {
    if (pkt.getFlag(Packet::PASSED_CORE) == 0) {
        asSrcToR(pkt);
    } else asDstToR(pkt);
    pkt.sendOn();
}

void ToR::asSrcToR(Packet &pkt) {
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_TOR_SRC);
    /* pkt.route = ... - dstToR - pServerToR - qServerToR - tcpSink */
    ToR *dstTor = dynamic_cast<ToR *> ((*(pkt.getRoute()))[pkt.getRoute()->size() - 4]);
    uint8_t dstToRId = dstTor->idTor;
    pkt.vxlan.src = idTor;
    pkt.vxlan.dst = dstToRId;
    pkt.vxlan.lbtag = minCongestedCore(dstToRId);
    pkt.vxlan.ce = 0; 
    pkt.vxlan.fb_lbtag = nxtLbTag[dstToRId];
    pkt.vxlan.fb_ce = CongFromLeaf[dstToRId][pkt.vxlan.fb_lbtag];
    updateNxtLbTag(dstToRId);
    congaRoute::updateRoute(&pkt, pkt.vxlan.lbtag);
    if (_logger) {
        _logger->logTxt("ToR" + to_string(idTor) + ": " + "chosen core " + to_string(pkt.vxlan.lbtag) + "for pkt id " + to_string(pkt.id()) + "\n");
    }
}

void ToR::asDstToR(Packet &pkt) {
    pkt.flow().logTraffic(pkt, *this, TrafficLogger::PKT_TOR_DST);
    CongToLeaf[pkt.vxlan.src][pkt.vxlan.fb_lbtag] = pkt.vxlan.fb_ce;

    CongFromLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
    nxtLbTag[pkt.vxlan.src] = pkt.vxlan.lbtag;
    
    if (_logger) {
        _logger->logTxt(congTableDump(true));
        _logger->logTxt(congTableDump(false));
    } 
}

std::string ToR::congTableDump(bool from) {
    std::string d("");
    d += (from? "CongFromLeaf\n": "CongToLeaf\n");

    for(uint8_t i = 0; i < ToR::N_TOR; i++) {
        d += (to_string(i) + "\t");
        for(uint8_t j = 0; j < CoreQueue::N_CORE; j++) {
            d += (to_string(from? CongFromLeaf[i][j]: CongToLeaf[i][j]) + "\t");
        }
        d += "\n";
    }

    return d;
}

uint8_t ToR::minCongestedCore(uint8_t dstToR) {
    uint8_t minCore = 0;
    uint8_t minCg = CongToLeaf[dstToR][0];
    for (uint8_t i = 1; i < CoreQueue::N_CORE; i++) {
        if (CongToLeaf[dstToR][i] == minCg) {
            double toss = rand() * 1.0 / RAND_MAX;
            if (toss < 0.5)
                minCore = i;
        } else if (CongToLeaf[dstToR][i] < minCg) {
            minCore = i;
            minCg = CongToLeaf[dstToR][i];
        } 
    }
    return minCore;
}

void ToR::updateNxtLbTag(uint8_t dstToR) {
    nxtLbTag[dstToR] = (nxtLbTag[dstToR] + 1) % CoreQueue::N_CORE;
}