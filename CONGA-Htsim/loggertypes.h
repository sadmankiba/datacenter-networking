/*
 * Logger type header
 */
#ifndef LOGGERTYPES_H
#define LOGGERTYPES_H

#include "logfile.h"
#include "logged.h"

#include <string>

class Packet;
class TcpSrc;
class Queue;

class Logger
{
    friend class Logfile;
public:
    void setLogfile(Logfile &logfile) { _logfile = &logfile; }
    void logTxt(std::string txt) {if (_logfile)  _logfile->writeTxt(txt); }
protected:
    Logfile *_logfile;
};



class TrafficLogger: public Logger
{
public:
    enum EventType { TRAFFIC_EVENT = 3 };
    enum TrafficEvent {
        PKT_ARRIVE     = 0,
        PKT_DEPART     = 1,
        PKT_CREATESEND = 2,
        PKT_DROP       = 3,
        PKT_RCVDESTROY = 4,
        PKT_TOR_SRC    = 5,
        PKT_TOR_DST    = 6
    };

    virtual void logTraffic(Packet &pkt, Logged &location, TrafficEvent ev) = 0;
    virtual ~TrafficLogger(){};
};

class QueueLogger: public Logger
{
public:
    enum EventType {
        QUEUE_EVENT  = 0,
        QUEUE_RECORD = 4,
        QUEUE_APPROX = 5
    };
    enum QueueEvent {
        PKT_ENQUEUE = 0,
        PKT_DROP    = 1,
        PKT_SERVICE = 2
    };
    enum QueueRecord { CUM_TRAFFIC = 0 };
    enum QueueApprox {
        QUEUE_RANGE    = 0,
        QUEUE_OVERFLOW = 1
    };

    virtual void logQueue(Queue &queue, QueueEvent ev, Packet &pkt) = 0;
    virtual ~QueueLogger(){};
};

class TcpLogger: public Logger
{
public:
    enum EventType {
        TCP_EVENT  = 1,
        TCP_STATE  = 2,
        TCP_RECORD = 6,
        TCP_SINK   = 11
    };
    enum TcpEvent {
        TCP_RCV              = 0,
        TCP_RCV_FR_END       = 1,
        TCP_RCV_FR           = 2,
        TCP_RCV_DUP_FR       = 3,
        TCP_RCV_DUP          = 4,
        TCP_RCV_3DUPNOFR     = 5,
        TCP_RCV_DUP_FASTXMIT = 6,
        TCP_TIMEOUT          = 7,
    };
    enum TcpState {
        TCPSTATE_CNTRL = 0,
        TCPSTATE_SEQ   = 1
    };
    enum TcpRecord { AVE_CWND = 0 };
    enum TcpSinkRecord { RATE = 0 };

    virtual void logTcp(TcpSrc &src, TcpEvent ev) = 0;
    virtual ~TcpLogger(){};
};

#endif /* LOGGERTYPES_H */
