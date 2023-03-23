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

    route_t rfwd, rbck;
    void routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst);
}

using namespace std;
using namespace conga;



void
conga_testbed(const ArgList &args, Logfile &logfile)
{
    // 4 link testbed
    rfwd.push_back(srcQ);
    DataSource::EndHost eh = DataSource::TCP;
    linkspeed_bps flowRate = 1000000000; // 1Gb
    uint32_t avgFlowSize = MSS_BYTES * 10;
    Workloads::FlowDist flowSizeDist = Workloads::UNIFORM;
    FlowGenerator(eh, routeGenerate, flowRate, avgFlowSize, flowSizeDist);

    Pipe *pipefwd = new Pipe(timeFromUs(10));
    pipefwd->setName("pipefwd");
    Pipe *pipebck = new Pipe(timeFromUs(8));
    pipebck->setName("pipebck");

    logfile.writeName(pipefwd->id, pipefwd->str());
    logfile.writeName(pipebck->id, pipebck->str());

    Queue *qfwd = new FairQueue(1000, 40, nullptr);
    qfwd->setName("qfwd");
    Queue *qbck = new FairQueue(2400, 30, nullptr);
    qbck->setName("qbck");

    logfile.writeName(qfwd->id, qfwd->str());
    logfile.writeName(qbck->id, qbck->str());

    rfwd.push_back(qfwd);
    rfwd.push_back(pipefwd);
    rbck.push_back(qbck);
    rbck.push_back(pipebck);
    
    DataSource::EndHost eh = DataSource::TCP;
    Workloads::FlowDist fd = Workloads::UNIFORM;

    FlowGenerator *fg = new FlowGenerator(eh, routeGenerate, 200, 60, fd); 

    fg->setEndhostQueue(500, 25);
    fg->setTimeLimits(0, timeFromUs(200) - 1);

    EventList::Get().setEndtime(timeFromUs(200));
}

void conga::routeGenerate(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst) {
    fwd = new route_t(rfwd); rev = new route_t(rbck); 
    src = 0; dst = 1;
}