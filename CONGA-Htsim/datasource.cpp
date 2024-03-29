/*
 * DataSource
 */
#include "datasource.h"

using namespace std;

DataSource::DataSource(TrafficLogger *logger, 
                       uint64_t flowsize, 
                       simtime_picosec duration)
                      : EventSource("datasource"),
                      _flowsize(flowsize),
                      _duration(duration),
                      _start_time(0),
                      _deadline(0),
                      _packets_sent(0),
                      _highest_sent(0),
                      _last_acked(0),
                      _enable_deadline(false),
                      _flowgen(NULL),
                      _flow(logger)
{
    // constructor
}

void 
DataSource::setFlowGenerator(FlowGenerator *flowgen)
{
    _flowgen = flowgen;
}

void 
DataSource::setDeadline(simtime_picosec deadline)
{
    _deadline = deadline;
}

/* */
void 
DataSource::connect(simtime_picosec start_time, 
                    route_t &route_fwd, 
                    route_t &route_rev, 
                    DataSink &sink)
{
    _route_fwd = &route_fwd;
    _route_rev  = &route_rev;

    _start_time = start_time;

    _sink = &sink;

    _flow.id = id; // identify the packet flow with the datasource that generated it

    _flow.logTxt("Flow start: " + str() + " " + to_string(timeAsUs(_start_time)) + " " 
        + to_string(id) + " " + to_string(_flowsize) + " " + to_string(_node_id) + " " + to_string(_sink->_node_id) + "\n");

    _sink->connect(*this, *_route_rev);
    EventList::Get().sourceIsPending(*this, _start_time);
}
