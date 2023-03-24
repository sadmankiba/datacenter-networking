#ifndef CONGA_H
#define CONGA_H

#include "network.h"
#include "queue.h"
#include <vector>

using namespace std;

class ToR: public PacketSink {
public:
    ToR() {
        idTor = TOR_ADDED;
        ToR::TOR_ADDED++;
        for(int i = 0; i < 256; i++)
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
    void receivePacket(Packet &pkt) {
        if (packet.getFlag(Packet::PASSED_CORE) == 0) {
            asSrcToR(pkt)
        } else asDstToR(pkt)
    }

    void asSrcToR(Packet &pkt) {
        pkt.flow().logTraffic(pkt, *this, ToRLog::TOR_SRC);

        if (pkt.getFlag(Packet::ACK) == 0) {
            pkt.vxlan.lbtag = 0; // uplink_port, set from route or less cong
            pkt.vxlan.ce = 0; 
        } else {
            uint8_t dstToRId = ((ToR) (*(pkt.getRoute())[pkt.getRoute()->size() - 4])).idTor;
            pkt.vxlan.src = idTor;
            pkt.vxlan.dst = dstToRId;
            pkt.vxlan.lbtag = nxtLbTag[dstToRId];
            pkt.vxlan.ce = CongFromLeaf[dstToRId][pkt.vxlan.lbtag];
        }
    }

    void asDstToR(Packet &pkt) {
        pkt.flow().logTraffic(pkt, *this, ToRLog::TOR_DST);
        if(pkt.getFlag(Packet::ACK) == 0) {
            CongFromLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
        } else {
            CongToLeaf[pkt.vxlan.src][pkt.vxlan.lbtag] = pkt.vxlan.ce;
        }
    }

    uint8_t idTor;
    const static uint8_t N_TOR = 2;
private:
    vector<uint8_t> nxtLbTag;
    vector<vector<uint8_t>> CongFromLeaf;
    vector<vector<uint8_t>> CongToLeaf;
    static uint8_t TOR_ADDED;
}

class CoreQueue: public Queue {
public:
    CoreQueue(linkspeed_bps bitrate, mem_b maxsize, QueueLogger *logger):
        Queue(bitrate, maxsize, logger) { 
        for(uint8_t i = 0; i < ToR::N_TOR; i++) {
            regCong.push_back(0);
        }
    }

    void receivePacket(Packet &pkt) {
        uint8_t egPort = ((ToR) (*(pkt.getRoute())[pkt.getNextHop()])).idToR;
            
        if (pkt.getFlag(Packet::ACK) == 0) {
            if (regCong[egPort] > pkt.vxlan.ce) {
                pkt.vxlan.ce = (uint8_t) (regCong[egPort] * 8);
            }
        }
        updateRegCong(egPort);
        pkt.setFlag(Packet::PASSED_CORE);
        if (_logger)
            _logger.logPacket(pkt);
        Queue::receivePacket(pkt);
    }

    const static uint8_t N_CORE = 1;
private:
    void updateRegCong(uint8_t egPort) {
        if (regCong[egPort] - 0 < 1e-8) 
            regCong[egPort] = _queuesize * 1.0 / _maxsize;
        else {
            regCong[egPort] = regCong[egPort] * 7 / 8 + (_queuesize * 1.0 / _maxsize) * 1 / 8;
        }
    }
    vector<double> regCong(ToR::N_TOR);
}

#endif 
