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
    const int N_CORE = CoreQueue::N_CORE; // 12
    const int N_LEAF = ToR::N_TOR; // 24
    const int N_SERVER = 32;   // Per leaf

    const uint64_t LEAF_BUFFER = 512000;
    const uint64_t CORE_BUFFER = 1024000;
    const uint64_t ENDH_BUFFER = 8192000;

    const uint64_t LEAF_SPEED = 10000000000; // 10gbps
    const uint64_t CORE_SPEED = 40000000000; // 40gbps

    Queue *qServerToR[N_LEAF];
    Pipe *pServerToR[N_LEAF];

    Queue *qToRServer[N_LEAF];
    Pipe *pToRServer[N_LEAF];
    
    Queue *qToRCore[N_LEAF];
    Pipe *pToRCore[N_LEAF];

    CoreQueue *qCoreToR[N_LEAF];
    Pipe *pCoreToR[N_LEAF];

    ToR *tor[N_LEAF];

    route_t rfwd, rrev;
    void routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst);
}

using namespace std;
using namespace conga;

void
conga_testbed(const ArgList &args, Logfile &logfile)
{
    vector<Queue *> vQ{qServerToR, qToRServer, qToRCore, qCoreToR};
    vector<string> nameV{"qServerToR", "qToRServer", "qToRCore", "qCoreToR"};

    for (int j = 0; j < 4; j++) {
        for(int i = 0; i < N_LEAF; i++) {
            vQ[j][i] = new Queue(1000000000, 10000, nullptr);
            vQ[j][i]->setName(nameV[j] + to_string(i));
            logfile->writeName(vQ[j]->id, vQ[j]->str());
        }
    }

    vector<Pipe *> vP{pServerToR, pToRServer, pToRCore, pCoreToR};
    vector<string> nameP{"pServerToR", "pToRServer", "pToRCore", "pCoreToR"};

    for (int j = 0; j < 4; j++) {
        for(int i = 0; i < N_LEAF; i++) {
            vP[j][i] = new Pipe(timeFromUs(6));
            vP[j][i]->setName(nameP[j] + to_string(i));
            logfile->writeName(vP[j]->id, vP[j]->str());
        }
    }

    for (int i =0; i < N_LEAF; i++) {
        tor[i] = new ToR();
        tor[i]->setName("ToR" + to_string(i));
        logfile->writeName(tor[i]->id, tor[i]->str());
    }

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
    rfwd.push_back(qToRCore[src]);
    rfwd.push_back(pToRCore[src]);
    rfwd.push_back(qCoreToR[dst]);
    rfwd.push_back(pCoreToR[dst]);
    rfwd.push_back(tor[dst]);
    rfwd.push_back(qToRServer[dst]);
    rfwd.push_back(pToRServer[dst]);

    rrev.push_back(qServerToR[dst]);
    rrev.push_back(pServerToR[dst]);
    rrev.push_back(tor[dst]);
    rrev.push_back(qToRCore[dst]);
    rrev.push_back(pToRCore[dst]);
    rrev.push_back(qCoreToR[src]);
    rrev.push_back(pCoreToR[src]);
    rrev.push_back(tor[src]);
    rrev.push_back(qToRServer[src]);
    rrev.push_back(pToRServer[src]);

    fwd = new route_t(rfwd); rev = new route_t(rrev); 
}