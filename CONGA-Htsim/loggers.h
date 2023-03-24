/*
 * Loggers header
 */
#ifndef LOGGERS_H
#define LOGGERS_H

#include "eventlist.h"
#include "logfile.h"
#include "loggertypes.h"
#include "network.h"
#include "queue.h"
#include "tcp.h"
#include "prof.h"

#include <vector>
#include <unordered_map>

class QueueLoggerSimple : public Logger, public QueueLogger
{
    public:
        void logQueue(Queue& queue, QueueLogger::QueueEvent ev, Packet& pkt)
        {
            _logfile->writeRecordWithTxt(QueueLogger::QUEUE_EVENT, queue.id, _queueEvMap[ev], ev,
                    "queuesize", (double)queue._queuesize, "flow id", pkt.flow().id, "pkt id", pkt.id());
        }

        void logPacket(Packet &pkt){
            _logfile->writePktTxt(pkt);
        }
    private:
        std::unordered_map<uint8_t, std::string> _queueEvMap = {{0, "PKT_ENQUEUE"}, {1, "PKT_DROP"}, {2, "PKT_SERVICE"}};
};

class TrafficLoggerSimple : public TrafficLogger
{
    public:
        void logTraffic(Packet& pkt, Logged& location, TrafficEvent ev)
        {
            _logfile->writeRecordWithTxt(TrafficLogger::TRAFFIC_EVENT, location.id,
                   _trafficEvMap[ev], ev, "flow id", pkt.flow().id, "pkt id", pkt.id(), "unused", 0);
        }
    private: 
        std::unordered_map<uint8_t, std::string> _trafficEvMap = {{0, "PKT_ARRIVE"}, {1, "PKT_DEPART"}, {2, "PKT_CREATESEND"}, 
                {3, "PKT_DROP"}, {4, "PKT_RCVDESTROY"}};
};

class TcpLoggerSimple : public TcpLogger
{
    public:
        void logTcp(TcpSrc &tcp, TcpEvent ev)
        {
            _logfile->writeRecordWithTxt(TcpLogger::TCP_EVENT, tcp.id, _tcpEvMap[ev], ev, "cwnd", tcp._cwnd,
                   "sendbuf size", tcp._highest_sent - tcp._last_acked, "TCP state", tcp._state);

            _logfile->writeRecordWithTxt(TcpLogger::TCP_STATE, tcp.id, "TCPSTATE_CNTRL", TcpLogger::TCPSTATE_CNTRL,
                    "cwnd", tcp._cwnd, "ssthresh", tcp._ssthresh, "recover seq", tcp._recover_seq);

            _logfile->writeRecordWithTxt(TcpLogger::TCP_STATE, tcp.id, "TCPSTATE_SEQ", TcpLogger::TCPSTATE_SEQ,
                    "last_acked", tcp._last_acked, "highest_sent", tcp._highest_sent, "RTO timeout", tcp._RFC2988_RTO_timeout);
        }
    private:
        std::unordered_map<uint8_t, std::string> _tcpEvMap = {{0, "TCP_RCV"},
        {1, "TCP_RCV_FR_END"},
        {2, "TCP_RCV_FR"},
        {3, "TCP_RCV_DUP_FR"},
        {4, "TCP_RCV_DUP"},
        {5, "TCP_RCV_3DUPNOFR"},
        {6, "TCP_RCV_DUP_FASTXMIT"},
        {7, "TCP_TIMEOUT"}};
};

class QueueLoggerSampling : public Logger, public QueueLogger, public EventSource
{
    public:
        QueueLoggerSampling(simtime_picosec period);
        void logQueue(Queue& queue, QueueEvent ev, Packet& pkt);
        void doNextEvent();
    private:
        Queue* _queue;
        simtime_picosec _lastlook;
        simtime_picosec _period;
        mem_b _lastq;
        bool _seenQueueInD;
        mem_b _minQueueInD;
        mem_b _maxQueueInD;
        mem_b _lastDroppedInD;
        mem_b _lastIdledInD;
        int _numIdledInD;
        int _numDropsInD;
        double _cumidle;
        double _cumarr;
        double _cumdrop;
        uint32_t _bytesInD;
        bool _printed;
};

class SinkLoggerSampling : public Logger, public EventSource
{
    public:
        SinkLoggerSampling(simtime_picosec period);
        void doNextEvent();
        void monitorSink(DataSink *sink);
    private:
        std::vector<DataSink *> _sinks;
        std::vector<uint64_t> _last_seq;
        std::vector<double> _last_rate;

        simtime_picosec _last_time;
        simtime_picosec _period;
};

#if MING_PROF
class TcpLoggerSampling : public TcpLogger, public EventSource
{
    public:
        TcpLoggerSampling(simtime_picosec period);
        void logTcp(TcpSrc &tcp, TcpEvent ev);
        void doNextEvent();
    private:
        TcpSrc *_tcp;
        simtime_picosec _period;
};
#endif

class AggregateTcpLogger : public Logger, public EventSource
{
    public:
        AggregateTcpLogger(simtime_picosec period);
        void doNextEvent();
        void monitorTcp(TcpSrc& tcp);
    private:
        simtime_picosec _period;
        typedef std::vector<TcpSrc*> tcplist_t;
        tcplist_t _monitoredTcps;
};

#endif /* LOGGERS_H */
