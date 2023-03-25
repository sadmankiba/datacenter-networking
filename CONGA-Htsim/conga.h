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
    ToR(Logger *logger);
    void receivePacket(Packet &pkt);
    void asSrcToR(Packet &pkt);
    void asDstToR(Packet &pkt);
    uint8_t minCongestedCore(uint8_t dstToR);
    void updateNxtLbTag(uint8_t dstToR);
    std::string congTableDump(bool from);

    uint8_t idTor;
    const static uint8_t N_TOR = 2;
private:
    vector<uint8_t> nxtLbTag;
    vector<vector<uint8_t>> CongFromLeaf;
    vector<vector<uint8_t>> CongToLeaf;
    static uint8_t TOR_ADDED;
    Logger *_logger;
};

class CoreQueue: public Queue {
public:
    CoreQueue(uint8_t coreId, uint8_t torId, linkspeed_bps bitrate, mem_b maxsize, QueueLogger *logger):
        Logged("CoreQueue"), Queue(bitrate, maxsize, logger),
        _coreId(coreId), _torId(torId) { 
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
        pkt.setFlag(Packet::PASSED_CORE);
        if (_logger) {
            _logger->logTxt("Core " + to_string(_coreId) + "," + to_string(_torId) + ": ");
            _logger->logTxt(pkt.dump());
        }
        Queue::receivePacket(pkt);
        updateRegCong(egPort);
        if (_logger) {
            _logger->logTxt(regCongDump());
        }
    }

    const static uint8_t N_CORE = 2;
private:
    std::string regCongDump() {
        std::string d("");
        d += "queuesize: " + to_string(_queuesize);
        d += " regCong: ";
        for (int i = 0; i < ToR::N_TOR; i++) {
            d += (to_string(regCong[i]) + " ");
        }

        d += "\n";
        return d;
    }
    void updateRegCong(uint8_t egPort) {
        if ((regCong[egPort] - 0) < 1e-5) 
            regCong[egPort] = (_queuesize * 1.0) / _maxsize;
        else {
            regCong[egPort] = (regCong[egPort] * 7.0) / 8 + (_queuesize * 1.0 / _maxsize) * 1 / 8;
        }
    }
    vector<double> regCong;
    uint8_t _coreId;
    uint8_t _torId;
};

#endif 
