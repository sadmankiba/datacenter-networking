/*
 * DataSink header
 */
#ifndef DATASINK_H
#define DATASINK_H

#include "loggertypes.h"
#include "datapacket.h"

#include <list>

class DataSource;
/*
 * A bare minimum TCP sink. 
 * Tracks ordered received seq num. Also, tracks out-of-order received packets in an infinite buffer.
 * Does not drop packet and reply ACK packets. 
 * Does not generate any log.
 */
class DataSink : public PacketSink
{
    public:
        DataSink();
        virtual ~DataSink() {};

        virtual void receivePacket(Packet &pkt) = 0;

        void connect(DataSource &src, route_t &route);
        void processDataPacket(DataPacket &pkt);

        DataAck::seq_t cumulative_ack();
        uint32_t drops();

        DataAck::seq_t _cumulative_ack;
        std::list<std::pair<DataAck::seq_t,mem_b> > _received;

        uint32_t _node_id;

    protected:
        DataSource *_src;
        route_t *_route;  // Back to source.
};

#endif /* DATASINK_h*/
