#ifndef CONGA_H
#define CONGA_H

#include "network.h"
#include "queue.h"
#include <vector>

using namespace std;

class CoreQueue;
class ToR;
namespace congaRoute {
    void updateRoute(Packet *pkt, uint8_t core);
}

class ToR: public PacketSink {
public:
    ToR();
    void receivePacket(Packet &pkt);

    void asSrcToR(Packet &pkt);

    void asDstToR(Packet &pkt);

    uint8_t minCongestedCore(uint8_t dstToR);

    void updateNxtLbTag(uint8_t dstToR);

    uint8_t idTor;
    const static uint8_t N_TOR = 2;
private:
    vector<uint8_t> nxtLbTag;
    vector<vector<uint8_t>> CongFromLeaf;
    vector<vector<uint8_t>> CongToLeaf;
    static uint8_t TOR_ADDED;
};

class CoreQueue: public Queue {
public:
    CoreQueue(linkspeed_bps bitrate, mem_b maxsize, QueueLogger *logger):
        Logged("CoreQueue"), Queue(bitrate, maxsize, logger) { 
        for(uint8_t i = 0; i < ToR::N_TOR; i++) {
            regCong.push_back(0);
        }
    }

    void receivePacket(Packet &pkt) {
        uint8_t egPort = ((ToR *) ((*(pkt.getRoute()))[pkt.getNextHop()]))->idTor;
            
        if (pkt.getFlag(Packet::ACK) == 0) {
            if (regCong[egPort] > pkt.vxlan.ce) {
                pkt.vxlan.ce = (uint8_t) (regCong[egPort] * 8);
            }
        }
        updateRegCong(egPort);
        pkt.setFlag(Packet::PASSED_CORE);
        if (_logger)
            _logger->logTxt(pkt.dump());
        Queue::receivePacket(pkt);
    }

    const static uint8_t N_CORE = 2;
private:
    void updateRegCong(uint8_t egPort) {
        if (regCong[egPort] - 0 < 1e-8) 
            regCong[egPort] = _queuesize * 1.0 / _maxsize;
        else {
            regCong[egPort] = regCong[egPort] * 7 / 8 + (_queuesize * 1.0 / _maxsize) * 1 / 8;
        }
    }
    vector<double> regCong;
};

#endif 
