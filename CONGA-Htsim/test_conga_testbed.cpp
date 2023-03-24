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

namespace conga {

    // tesdbed configuration
    const int N_CORE = 12;
    const int N_LEAF = 24;
    const int N_SERVER = 32;   // Per leaf

    const uint64_t LEAF_BUFFER = 512000;
    const uint64_t CORE_BUFFER = 1024000;
    const uint64_t ENDH_BUFFER = 8192000;

    const uint64_t LEAF_SPEED = 10000000000; // 10gbps
    const uint64_t CORE_SPEED = 40000000000; // 40gbps

    Queue *qServerToR[2];
    Pipe *pServerToR[2];

    Queue *qToRServer[2];
    Pipe *pToRServer[2];
    
    Queue *qToRCore[2];
    Pipe *pToRCore[2];

    Queue *qCoreToR[2];
    Pipe *pCoreToR[2];

    ToR *tor[2];
    CoreRouter *core;

    route_t rfwd, rrev;
    void routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst);
}

using namespace std;
using namespace conga;

void
conga_testbed(const ArgList &args, Logfile &logfile)
{
    vector<Queue **> vQ = {&qServerToR, &qToRServer, &qToRCore, &qCoreToR};
    for (Queue **Q: vQ)
        for(int i = 0; i < 2; i++)
            Q[i] = new Queue(1000000000, 10000, nullptr);

    vector<Pipe **> vP = {&pServerToR, &pToRServer, &pToRCore, &pCoreToR};
    for (Pipe **P: vP)
        for(int i = 0; i < 2; i++)
            P[i] = new Pipe(timeFromUs(6));
    
    for (int i =0; i < 2; i++)
        tor[i] = new ToR();
    
    core = new CoreRouter();

    

    DataSource::EndHost eh = DataSource::TCP;
    linkspeed_bps flowRate = 1000000000; // 1Gb
    uint32_t avgFlowSize = MSS_BYTES * 10;
    Workloads::FlowDist flowSizeDist = Workloads::UNIFORM;
    FlowGenerator *fg = new FlowGenerator(eh, routeGenerate, flowRate, avgFlowSize, flowSizeDist);

    fg->setTimeLimits(0, timeFromUs(4000) - 1);

    EventList::Get().setEndtime(timeFromUs(4000));
}

void conga::routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst) {
    src = 0;
    dst = 1;
    // 4 link testbed
    rfwd.push_back(qServerToR[src]);
    rfwd.push_back(pServerToR[src]);
    rfwd.push_back(tor[src]);
    tor[0]->dstToRId = 1;
    rfwd.push_back(qToRCore[0]);
    rfwd.push_back(pToRCore[0]);
    rfwd.push_back(core);
    rfwd.push_back(qCoreToR[1]);
    rfwd.push_back(pCoreToR[1]);
    rfwd.push_back(tor[1]);
    rfwd.push_back(qToRServer[1]);
    rfwd.push_back(pToRServer[1]);

    rrev.push_back(qServerToR[1]);
    rrev.push_back(pServerToR[1]);
    rrev.push_back(tor[1]);
    rrev.push_back(qToRCore[1]);
    rrev.push_back(pToRCore[1]);
    rrev.push_back(core);
    rrev.push_back(qCoreToR[0]);
    rrev.push_back(pCoreToR[0]);
    rrev.push_back(tor[0]);
    rrev.push_back(qToRServer[0]);
    rrev.push_back(pToRServer[0]);

    fwd = new route_t(rfwd); rev = new route_t(rrev); 
}