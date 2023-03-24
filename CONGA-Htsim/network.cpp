/*
 * Network
 */
#include "network.h"

using namespace std;

uint32_t Logged::LASTIDNUM = 1;

void
Packet::set(PacketFlow &flow,
            route_t &route,
            mem_b pkt_size,
            packetid_t id)
{
    _flow = &flow;
    _route = &route;
    _size = pkt_size;
    _id = id;
    _nexthop = 0;
    _flags = 0;
    _priority = 0;
}

void
Packet::sendOn()
{
    assert(_nexthop<_route->size());

    PacketSink *nextsink = (*_route)[_nexthop];
    _nexthop++;

    nextsink->receivePacket(*this);
}

std::string Packet::dump() {
    std::string dumpStr("");

    dumpStr += ("Packet: ack " + to_string(getFlag(Packet::ACK)) + ", passed_core " + to_string(getFlag(Packet::PASSED_CORE)) + ", id " + to_string(id())\
            + ", nxthop " + to_string(getNextHop()) + ", vxlan-(src " + to_string(vxlan.src)\
            + " dst " + to_string(vxlan.dst) + " lbtag " + to_string(vxlan.lbtag) + " ce " + to_string(vxlan.ce) + ")");
    dumpStr += ", route ";
    route_t route = *(getRoute());
    for (PacketSink * sink: route) 
        dumpStr += (sink->str() + "-");
    dumpStr += "\n";
    return dumpStr;
}

PacketFlow::PacketFlow(TrafficLogger *logger)
                      : Logged("PacketFlow"), 
                      _nPackets(0), 
                      _logger(logger)
{}

void
PacketFlow::logTraffic(Packet &pkt, 
                       Logged &location, 
                       TrafficLogger::TrafficEvent ev)
{   
    if (_logger) {
        _logger->logTraffic(pkt, location, ev);
    }
}
