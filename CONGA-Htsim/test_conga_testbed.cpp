#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "aprx-fairqueue.h"
#include "fairqueue.h"
#include "priorityqueue.h"
#include "stoc-fairqueue.h"
#include "flow-generator.h"
#include "pipe.h"
#include "test.h"
#include "conga.h"

#include <stdlib.h>

namespace conga {

    // tesdbed configuration
    const int N_CORE = CoreQueue::N_CORE; // 12
    const int N_LEAF = ToR::N_TOR; // 24
    const int N_SERVER = 1;   // 32, Per leaf

    const uint64_t LEAF_BUFFER = 512000;
    const uint64_t CORE_BUFFER = 1024000;
    const uint64_t ENDH_BUFFER = 8192000;

    const uint64_t LEAF_SPEED = 10000000000; // 10gbps
    const uint64_t CORE_SPEED = 40000000000; // 40gbps
    bool ecmp = false;

    Queue *qToRCore[N_CORE][N_LEAF];
    Pipe *pToRCore[N_CORE][N_LEAF];

    CoreQueue *qCoreToR[N_CORE][N_LEAF];
    Pipe *pCoreToR[N_CORE][N_LEAF];

    Queue *qServerToR[N_LEAF][N_SERVER];
    Pipe *pServerToR[N_LEAF][N_SERVER];

    Queue *qToRServer[N_LEAF][N_SERVER];
    Pipe *pToRServer[N_LEAF][N_SERVER];
        
    ToR *tor[N_LEAF];

    route_t rfwd, rrev;
    void routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst);
    uint8_t simpleHash(uint32_t val1, uint32_t val2) {
        return (val1 + val2) % 256; 
    }
}

using namespace std;
using namespace conga;

void
conga_testbed(const ArgList &args, Logfile &logfile)
{
    srand(time(NULL));
    QueueLoggerSimple *qlogger = new QueueLoggerSimple();
    qlogger->setLogfile(logfile);
    for (int i = 0; i < N_CORE; i++) {    
        for (int j = 0; j < N_LEAF; j++) {
            qToRCore[i][j] = new CoreQueue(4000000000, 12000, qlogger);
            qCoreToR[i][j] = new CoreQueue(8000000000, 18000, qlogger);

            pToRCore[i][j] = new Pipe(timeFromUs(9));
            pCoreToR[i][j] = new Pipe(timeFromUs(10));

            qToRCore[i][j]->setName("qToRCore" + to_string(i) + "," + to_string(j));
            logfile.writeName(qToRCore[i][j]->id, qToRCore[i][j]->str());
            qCoreToR[i][j]->setName("qCoreToR" + to_string(i) + "," + to_string(j));
            logfile.writeName(qCoreToR[i][j]->id, qCoreToR[i][j]->str());

            pToRCore[i][j]->setName("pToRCore" + to_string(i) + "," + to_string(j));
            logfile.writeName(pToRCore[i][j]->id, pToRCore[i][j]->str());
            pCoreToR[i][j]->setName("pCoreToR" + to_string(i) + "," + to_string(j));
            logfile.writeName(pCoreToR[i][j]->id, pCoreToR[i][j]->str());
        }
    }

    for (int i = 0; i < N_LEAF; i++) {    
        for (int j = 0; j < N_SERVER; j++) {
            qServerToR[i][j] = new Queue(1000000000, 16000, qlogger);
            qToRServer[i][j] = new Queue(2000000000, 10000, qlogger);

            pServerToR[i][j] = new Pipe(timeFromUs(6));
            pToRServer[i][j] = new Pipe(timeFromUs(7));

            qServerToR[i][j]->setName("qServerToR" + to_string(i) + "," + to_string(j));
            logfile.writeName(qServerToR[i][j]->id, qServerToR[i][j]->str());
            qToRServer[i][j]->setName("qToRServer" + to_string(i) + "," + to_string(j));
            logfile.writeName(qToRServer[i][j]->id, qToRServer[i][j]->str());

            pServerToR[i][j]->setName("pServerToR" + to_string(i) + "," + to_string(j));
            logfile.writeName(pServerToR[i][j]->id, pServerToR[i][j]->str());
            pToRServer[i][j]->setName("pToRServer" + to_string(i) + "," + to_string(j));
            logfile.writeName(pToRServer[i][j]->id, pToRServer[i][j]->str());
        }
    }
    
    Logger *logger = new Logger();
    logger->setLogfile(logfile);
    for (int i =0; i < N_LEAF; i++) {
        tor[i] = new ToR(logger);
        tor[i]->setName("ToR" + to_string(i));
        logfile.writeName(tor[i]->id, tor[i]->str());
    }

    DataSource::EndHost eh = DataSource::TCP;
    linkspeed_bps flowRate = 5000000000; // 5Gb
    uint32_t avgFlowSize = MSS_BYTES * 10;
    Workloads::FlowDist flowSizeDist = Workloads::UNIFORM;
    FlowGenerator *fg = new FlowGenerator(eh, routeGenerate, flowRate, avgFlowSize, flowSizeDist);
    
    TrafficLoggerSimple *_pktlogger = new TrafficLoggerSimple();
    _pktlogger->setLogfile(logfile);
    fg->setTrafficLogger(_pktlogger);
    fg->setLogFile(&logfile);

    fg->setTimeLimits(0, timeFromUs(4000) - 1);

    EventList::Get().setEndtime(timeFromUs(4000));
}

void conga::routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst) {
    src = 0;
    dst = 1;
    uint8_t srcToR = src / N_SERVER;
    uint8_t dstToR = dst / N_SERVER;
    uint8_t sInRack = src % N_SERVER;
    uint8_t dInRack = dst % N_SERVER;
    uint8_t core = (simpleHash(src, rand())) % N_CORE; // ECMP-like
    // 4 link testbed
    rfwd.push_back(qServerToR[srcToR][sInRack]);
    rfwd.push_back(pServerToR[src][sInRack]);
    if (!ecmp) rfwd.push_back(tor[srcToR]);
    rfwd.push_back(qToRCore[core][srcToR]);
    rfwd.push_back(pToRCore[core][srcToR]);
    rfwd.push_back(qCoreToR[core][dstToR]);
    rfwd.push_back(pCoreToR[core][dstToR]);
    if (!ecmp) rfwd.push_back(tor[dstToR]);
    rfwd.push_back(qToRServer[dstToR][dInRack]);
    rfwd.push_back(pToRServer[dstToR][dInRack]);

    rrev.push_back(qServerToR[dstToR][dInRack]);
    rrev.push_back(pServerToR[dstToR][dInRack]);
    if (!ecmp) rrev.push_back(tor[dstToR]);
    rrev.push_back(qToRCore[core][dstToR]);
    rrev.push_back(pToRCore[core][dstToR]);
    rrev.push_back(qCoreToR[core][srcToR]);
    rrev.push_back(pCoreToR[core][srcToR]);
    if (!ecmp) rrev.push_back(tor[srcToR]);
    rrev.push_back(qToRServer[srcToR][sInRack]);
    rrev.push_back(pToRServer[srcToR][sInRack]);

    fwd = new route_t(rfwd); rev = new route_t(rrev); 
}

void congaRoute::updateRoute(Packet *pkt, uint8_t core) {
    pkt->updateRoute(pkt->getNextHop(), qToRCore[core][pkt->vxlan.src]);
    pkt->updateRoute(pkt->getNextHop() + 1, pToRCore[core][pkt->vxlan.src]);
    pkt->updateRoute(pkt->getNextHop() + 2, qCoreToR[core][pkt->vxlan.dst]);
    pkt->updateRoute(pkt->getNextHop() + 3, pCoreToR[core][pkt->vxlan.dst]);
}