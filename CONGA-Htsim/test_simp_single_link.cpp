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

namespace simp_single {
    route_t rfwd, rbck;
    void routeAssign(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst);
}

using namespace std;
using namespace simp_single;

void
simp_single_link(const ArgList &args, Logfile &logfile)
{
    Pipe *pipefwd = new Pipe(timeFromUs(10));
    pipefwd->setName("pipefwd");
    Pipe *pipebck = new Pipe(timeFromUs(8));
    pipebck->setName("pipebck");

    logfile.writeName(pipefwd->id, pipefwd->str());
    logfile.writeName(pipebck->id, pipebck->str());

    QueueLoggerSimple *ql = new QueueLoggerSimple();
    ql->setLogfile(logfile);
    Queue *qfwd = new Queue(1000000000, 10000, ql);
    qfwd->setName("qfwd");
    Queue *qbck = new Queue(1000000000, 4000, nullptr);
    qbck->setName("qbck");

    logfile.writeName(qfwd->id, qfwd->str());
    logfile.writeName(qbck->id, qfwd->str());

    rfwd.push_back(qfwd);
    rfwd.push_back(pipefwd);
    rbck.push_back(qbck);
    rbck.push_back(pipebck);
    
    DataSource::EndHost eh = DataSource::TCP;
    Workloads::FlowDist fd = Workloads::UNIFORM;
    TcpLoggerSimple *_tcplogger = new TcpLoggerSimple();
    _tcplogger->setLogfile(logfile);

    TrafficLoggerSimple *_pktlogger = new TrafficLoggerSimple();
    _pktlogger->setLogfile(logfile);

    // Avg flow arrival time = 1500 * 5 * 8 / 10^9 = 60 us
    FlowGenerator *fg = new FlowGenerator(eh, routeAssign, 1000000000, MSS_BYTES * 5, fd); 

    fg->setEndhostQueue(2000000000, 1000000);
    fg->setTrafficLogger(_pktlogger);
    fg->setTcpLogger(_tcplogger);
    fg->setTimeLimits(0, timeFromUs(200) - 1);

    EventList::Get().setEndtime(timeFromUs(200));
}

void simp_single::routeAssign(route_t *&fwd, route_t *&rev, uint32_t &src, uint32_t &dst) {
    fwd = new route_t(rfwd); rev = new route_t(rbck); 
    src = 0; dst = 1;
}