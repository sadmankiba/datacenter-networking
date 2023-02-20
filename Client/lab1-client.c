/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <rte_ip.h>
#include <rte_common.h>
#include "debug.h"

// #define PKT_TX_IPV4          (1ULL << 55)
// #define PKT_TX_IP_CKSUM      (1ULL << 54)

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 65535
#define MBUF_CACHE_SIZE 512
#define BUF_SIZE 128
#define TX_BURST_SIZE 1024
#define RX_BURST_SIZE 256
#define STOP_FLOW_ID 100
#define MAX_FLOW_NUM 8
#define TIMEOUT_US 50

struct rte_mbuf * construct_pkt(size_t, uint32_t);
size_t get_appl_data();
uint32_t get_seq(struct rte_mbuf *);

uint32_t NUM_PING;

/* Define the mempool globally */
struct rte_mempool *mbuf_pool = NULL;
static struct rte_ether_addr my_eth;
uint16_t eth_port_id = 1;

int flow_size;
int payload_len = 64;
int flow_num;


static uint64_t raw_time_ns(void) {
    struct timespec tstart={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    uint64_t t = (uint64_t)(tstart.tv_sec * 1.0e9 + tstart.tv_nsec);
    return t;

}

static uint64_t time_now_ns(uint64_t offset) {
    return raw_time_ns() - offset;
}

uint32_t
checksum(unsigned char *buf, uint32_t nbytes, uint32_t sum)
{
	unsigned int	 i;

	/* Checksum all the pairs of bytes first. */
	for (i = 0; i < (nbytes & ~1U); i += 2) {
		sum += (uint16_t)ntohs(*((uint16_t *)(buf + i)));
		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}

	if (i < nbytes) {
		sum += buf[i] << 8;
		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}

	return sum;
}

uint32_t
wrapsum(uint32_t sum)
{
	sum = ~sum & 0xFFFF;
	return htons(sum);
}

void print_mac(struct rte_ether_addr mac) {
    printf("MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			" %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
        mac.addr_bytes[0], mac.addr_bytes[1],
		mac.addr_bytes[2], mac.addr_bytes[3],
		mac.addr_bytes[4], mac.addr_bytes[5]);
}

uint64_t cycles_elapsed(double timediff) {
    return timediff * rte_get_timer_hz() / (1e9);
}

/**  
 * Parse a complete TCP packet.
 * 
 * @return
 *   - (0) if not a TCP packet
 *   - (+ve integer) denoting flow id
 */
static int parse_packet(uint64_t *recv_ack, uint32_t *rcv_wnd, struct rte_mbuf *pkt) {
    /* 
     * Packet layout order is (from outside -> in):
     * ether_hdr | ipv4_hdr | tcp_hdr | payload
     */
    uint8_t *p = rte_pktmbuf_mtod(pkt, uint8_t *);
    size_t header = 0;

    /* Parse ethernet header. Validate dst MAC and type is IP */
    struct rte_ether_hdr * const eth_hdr = (struct rte_ether_hdr *)(p);
    p += sizeof(*eth_hdr);
    header += sizeof(*eth_hdr);
    uint16_t eth_type = ntohs(eth_hdr->ether_type);
    struct rte_ether_addr mac_addr = {};

    rte_eth_macaddr_get(1, &mac_addr);
    if (!rte_is_same_ether_addr(&mac_addr, &eth_hdr->dst_addr)) {
        return 0;
    }
    if (RTE_ETHER_TYPE_IPV4 != eth_type) {
        return 0;
    }

    /* Parse IP header. Validate Type = TCP */
    struct rte_ipv4_hdr *const ip_hdr = (struct rte_ipv4_hdr *)(p);
    p += sizeof(*ip_hdr);
    header += sizeof(*ip_hdr);

    if (IPPROTO_TCP != ip_hdr->next_proto_id) {
        return 0;
    }
    
    /* Parse tcp header */
    struct rte_tcp_hdr * const tcp_hdr = (struct rte_tcp_hdr *)(p);
    p += sizeof(*tcp_hdr);
    header += sizeof(*tcp_hdr);

    int ret = 0;
	
	uint16_t p1 = rte_cpu_to_be_16(5001);
	uint16_t p2 = rte_cpu_to_be_16(5002);
	uint16_t p3 = rte_cpu_to_be_16(5003);
	uint16_t p4 = rte_cpu_to_be_16(5004);
    uint16_t p5 = rte_cpu_to_be_16(5005);
	uint16_t p6 = rte_cpu_to_be_16(5006);
	uint16_t p7 = rte_cpu_to_be_16(5007);
	uint16_t p8 = rte_cpu_to_be_16(5008);
	
	if (tcp_hdr->dst_port ==  p1) ret = 1;
	if (tcp_hdr->dst_port ==  p2) ret = 2;
	if (tcp_hdr->dst_port ==  p3) ret = 3;
	if (tcp_hdr->dst_port ==  p4) ret = 4;
    if (tcp_hdr->dst_port ==  p5) ret = 5;
	if (tcp_hdr->dst_port ==  p6) ret = 6;
	if (tcp_hdr->dst_port ==  p7) ret = 7;
	if (tcp_hdr->dst_port ==  p8) ret = 8;
    
    *recv_ack = tcp_hdr->recv_ack;
    *rcv_wnd = tcp_hdr->rx_win;
    return ret;
}


/* From basicfwd.c: Basic DPDK skeleton forwarding example.
 * 
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0)
	{
		printf("Error during getting device (port %u) info: %s\n",
			   port, strerror(-retval));
		return retval;
	}

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++)
	{
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
										rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++)
	{
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
										rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Starting Ethernet port. 8< */
	retval = rte_eth_dev_start(port);
	/* >8 End of starting of ethernet port. */
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	retval = rte_eth_macaddr_get(port, &my_eth);
	if (retval != 0)
		return retval;

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
		   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		   port, RTE_ETHER_ADDR_BYTES(&my_eth));

	/* Enable RX in promiscuous mode for the Ethernet device. */
	retval = rte_eth_promiscuous_enable(port);
	/* End of setting RX port in promiscuous mode. */
	if (retval != 0)
		return retval;

	return 0;
}

void debug_sendpkt(struct rte_mbuf *buf[], uint32_t n) {
	debug("sendpkt: ");
	
	for (uint32_t i = 0; i < n; i++) {
		debug("%u, ", get_seq(buf[i]));
	}
	debug("\n");
}

/* Send and receive packet on an Ethernet port. */
void lcore_main()
{
    /* Data structures for sending */
    struct rte_mbuf *pkts_rcvd[RX_BURST_SIZE];
    struct rte_mbuf *pkt;
    uint16_t nb_rx;
    size_t header_size;

    /* Sliding window variables */
    struct rte_mbuf *buf[MAX_FLOW_NUM][BUF_SIZE];
    struct rte_mbuf *send_pkts[BUF_SIZE];
    uint32_t last_pkt_acked[MAX_FLOW_NUM] = {0}; /* 1-indexed */
    uint32_t last_pkt_sent[MAX_FLOW_NUM] = {0};
    uint32_t last_pkt_written[MAX_FLOW_NUM] = {0};
    uint32_t recv_ack[MAX_FLOW_NUM] = {0};
    uint32_t n_empty_buf[MAX_FLOW_NUM];
    uint32_t retr[BUF_SIZE];
    uint64_t time_sent[MAX_FLOW_NUM][BUF_SIZE];
    uint32_t rcv_wnd[MAX_FLOW_NUM];
    uint16_t n_new_pkt[MAX_FLOW_NUM];
    uint32_t seq;

    for (uint8_t fid=0; fid < MAX_FLOW_NUM; fid++) {
        n_empty_buf[fid] = BUF_SIZE;
        rcv_wnd[fid] = BUF_SIZE;
        for (uint16_t i = 0; i < BUF_SIZE; i++) {
            time_sent[fid][i] = pow(2, 63);
            buf[fid][i] = NULL;
        }
    }
    
    /* Timing variables */
    uint64_t lat_cum_us[MAX_FLOW_NUM] = {0};
    uint64_t flow_start_time = raw_time_ns();
    uint64_t lat_ns;
    
    debug("To send %u packets per flow\n", NUM_PING);
    /* Repeatedly construct packet, retransmit, send data, recv ack. */

    while (true) {
        bool rpt = false;
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            if (last_pkt_acked[fid] < NUM_PING)
                rpt = true;
        }
        if(!rpt) break;
        
        debug("---New iter---\n"); 
        debug("last_pkt_acked: "); 
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            debug("%u, ", last_pkt_acked[fid]);
        }
        debug("last_pkt_sent: "); 
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            debug("%u, ", last_pkt_sent[fid]);
        }
        debug("last_pkt_written: "); 
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            debug("%u, ", last_pkt_written[fid]);
        }
        debug("\n");
        
        
        /* Retransmit unacked data */
        uint32_t n_snd;
        for (uint8_t fid = 0; fid < flow_num; fid++) {
            n_snd = 0;
            for (uint32_t i = last_pkt_acked[fid] + 1; 
                i <= last_pkt_sent[fid] && i <= last_pkt_written[fid]; i++) {
                if((raw_time_ns() - time_sent[fid][(i-1) % BUF_SIZE]) > (TIMEOUT_US * 1000)) { 
                    send_pkts[n_snd] = construct_pkt(fid, i);
                    if(false) {
                        send_pkts[n_snd] = buf[fid][(i - 1) % BUF_SIZE];
                    }
                    retr[n_snd] = i;
                    n_snd++;        
                }
            }
            if (n_snd > 0) {
                debug_sendpkt(send_pkts, n_snd);
                rte_eth_tx_burst(1, 0, send_pkts, n_snd);
                debug("retransmitted fid %u, n_snd %u\n", fid, n_snd);
            }
            for(uint16_t i = 0; i < n_snd; i++) {
                time_sent[fid][(retr[i] - 1) % BUF_SIZE] = raw_time_ns();
                rte_pktmbuf_free(send_pkts[i]);
            }
        }

        /* Populate buf to fill full */
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            last_pkt_written[fid] = NUM_PING < (last_pkt_acked[fid] + BUF_SIZE)? \
                NUM_PING : last_pkt_acked[fid] + BUF_SIZE;
        }

        /* Send data */
        uint32_t prev_sent;
        for (uint8_t fid = 0; fid < flow_num; fid++) {
            n_snd = 0;
            prev_sent = last_pkt_sent[fid];
            while(((last_pkt_sent[fid] - last_pkt_acked[fid]) < rcv_wnd[fid]) 
                && (last_pkt_sent[fid] < last_pkt_written[fid])) {
                    pkt = construct_pkt(fid, last_pkt_sent[fid] + 1);
                    send_pkts[n_snd++] = pkt;
                    last_pkt_sent[fid]++;
            }
            if(n_snd > 0) {
                debug_sendpkt(send_pkts, n_snd);
                rte_eth_tx_burst(1, 0, send_pkts, n_snd);
                for (uint32_t i = prev_sent + 1; i <= last_pkt_sent[fid]; i++)
                    time_sent[fid][(i - 1) % BUF_SIZE] = raw_time_ns();
                for(uint32_t i = 0; i < n_snd; i++)
                    rte_pktmbuf_free(send_pkts[i]);
                debug("Sent flow %u, n_snd %u, last pkt seq %u\n", fid, n_snd, last_pkt_sent[fid]);
            }
        }

        /* Recv data */
        uint32_t recv_ack_p, rcv_wnd_p;
        uint8_t rfid;
        uint32_t n_acked;
        while(true) {
            nb_rx = rte_eth_rx_burst(eth_port_id, 0, pkts_rcvd, RX_BURST_SIZE);
            uint64_t time_recvd = raw_time_ns();
            if(nb_rx == 0) break;
            
            for (int i = 0; i < nb_rx; i++) {
                int p = parse_packet(&recv_ack_p, &rcv_wnd_p, pkts_rcvd[i]);
                if (p != 0) {
                    rfid = p - 1;
                    recv_ack[rfid] = recv_ack_p;
                    rcv_wnd[rfid] = rcv_wnd_p;
                    if (recv_ack[rfid] > last_pkt_acked[rfid]) {
                        debug("Received fid %u, ack %u\n", rfid, recv_ack[rfid]);
                        n_acked = (recv_ack[rfid] - last_pkt_acked[rfid]);
                        n_empty_buf[rfid] += n_acked;
                        for(uint32_t i = last_pkt_acked[rfid] + 1; i <= recv_ack[rfid]; i++) {
                            rte_pktmbuf_free(buf[rfid][(i-1) % BUF_SIZE]);
                            buf[rfid][(i-1) % BUF_SIZE] = NULL;
                        }
                        lat_ns = time_recvd - time_sent[rfid][(recv_ack[rfid] - 1) % BUF_SIZE];
                        lat_cum_us[rfid] += (uint64_t) (lat_ns 
                            * (recv_ack[rfid] - last_pkt_acked[rfid]) * 1.0 / 1000);
                        last_pkt_acked[rfid] = recv_ack[rfid];
                        if(last_pkt_acked[rfid] > last_pkt_sent[rfid])
                            last_pkt_sent[rfid] = last_pkt_acked[rfid];
                    }
                }
                rte_pktmbuf_free(pkts_rcvd[i]);
            }
        }
        
        debug("lat_cum_us: "); 
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            debug("%lu, ", lat_cum_us[fid]);
        }
        debug("n_empty_buf: "); 
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            debug("%u, ", n_empty_buf[fid]);
        }
        debug("rcv_wnd: "); 
        for(uint8_t fid = 0; fid < flow_num; fid++) {
            debug("%u, ", rcv_wnd[fid]);
        }
        debug("\n");

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10 * 1000;
        nanosleep(&ts, NULL);
        // sleep(2);
        // if(last_pkt_acked > 0.25 * NUM_PING) printf("25% acked\n");
        // else if (last_pkt_acked > 0.5 * NUM_PING) printf("50% acked");

    }

    /* latency in us */
    double lat_agg = 0;
    for(uint8_t fid = 0; fid < flow_num; fid++) {
        lat_agg += (1.0 * lat_cum_us[fid]) / last_pkt_acked[fid];
    }
    printf("%f, ", lat_agg / flow_num); 
    
    /* Bw in Gbps */
    double bw_agg = 0;
    for(uint8_t fid = 0; fid < flow_num; fid++) {
        bw_agg += (1.0 * last_pkt_acked[fid] 
            * (header_size + payload_len) * 8) / time_now_ns(flow_start_time);
    }
    printf("%f", bw_agg / flow_num);

    /* Send special packet to shutdown receiver */
    pkt = construct_pkt(STOP_FLOW_ID, 0);
    rte_eth_tx_burst(eth_port_id, 0, &pkt, 1);
}

struct rte_mbuf * construct_pkt(size_t fid, uint32_t sent_seq) {
    /* Allocate a mbuf */
    struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);
    if (pkt == NULL) {
        printf("Error allocating tx mbuf\n");
        return NULL;
    }
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_tcp_hdr *tcp_hdr;

    /* Dst mac address */ 
    struct rte_ether_addr dst = {{0x14, 0x58, 0xd0, 0x58, 0x9f, 0x53}};

    size_t header_size = 0;

    /* 
     * Get a pointer to start of mbuf. Then, sequentially fill mbuf 
     * with Ethernet header, IP header, TCP header and payload. 
     * Finally, add metadata, such as l2_len and l3_len, in mbuf struct
     */
    uint8_t *ptr = rte_pktmbuf_mtod(pkt, uint8_t *);
    
    /* add in an ethernet header */
    eth_hdr = (struct rte_ether_hdr *)ptr;
    rte_ether_addr_copy(&my_eth, &eth_hdr->src_addr);
    rte_ether_addr_copy(&dst, &eth_hdr->dst_addr);
    eth_hdr->ether_type = rte_be_to_cpu_16(RTE_ETHER_TYPE_IPV4);
    ptr += sizeof(*eth_hdr);
    header_size += sizeof(*eth_hdr);

    /* add in ipv4 header */
    ipv4_hdr = (struct rte_ipv4_hdr *)ptr;
    ipv4_hdr->version_ihl = 0x45;
    ipv4_hdr->type_of_service = 0x0;
    ipv4_hdr->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr) + payload_len);
    ipv4_hdr->packet_id = rte_cpu_to_be_16(1);
    ipv4_hdr->fragment_offset = 0;
    ipv4_hdr->time_to_live = 64;
    ipv4_hdr->next_proto_id = IPPROTO_TCP;
    ipv4_hdr->src_addr = rte_cpu_to_be_32("127.0.0.1");
    ipv4_hdr->dst_addr = rte_cpu_to_be_32("127.0.0.1");

    uint32_t ipv4_checksum = wrapsum(checksum((unsigned char *)ipv4_hdr, sizeof(struct rte_ipv4_hdr), 0));
    ipv4_hdr->hdr_checksum = rte_cpu_to_be_32(ipv4_checksum);
    header_size += sizeof(*ipv4_hdr);
    ptr += sizeof(*ipv4_hdr);

    /* add in TCP hdr */
    tcp_hdr = (struct rte_tcp_hdr *) ptr;
    uint16_t srcp = 5001 + fid;
    uint16_t dstp = 5001 + fid;
    tcp_hdr->src_port = rte_cpu_to_be_16(srcp);
    tcp_hdr->dst_port = rte_cpu_to_be_16(dstp);
    tcp_hdr->sent_seq = sent_seq;
    tcp_hdr->recv_ack = 0;
    tcp_hdr->data_off = 0;
    tcp_hdr->tcp_flags = 0;
    tcp_hdr->rx_win = 0;
    tcp_hdr->cksum = 0;
    tcp_hdr->tcp_urp = 0;
    ptr += sizeof(*tcp_hdr);
    header_size += sizeof(*tcp_hdr);

    /* set the payload */
    memset(ptr, 'a', payload_len);

    pkt->l2_len = RTE_ETHER_HDR_LEN;
    pkt->l3_len = sizeof(struct rte_ipv4_hdr);
    pkt->data_len = header_size + payload_len;
    pkt->pkt_len = header_size + payload_len;
    pkt->nb_segs = 1;

    return pkt;
}

uint32_t get_seq(struct rte_mbuf *pkt) {
	if(pkt == NULL) return 0;

	uint8_t *p = rte_pktmbuf_mtod(pkt, uint8_t *);

	p += sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr);
	struct rte_tcp_hdr *tcp_hdr = (struct rte_tcp_hdr *) p;
	return tcp_hdr->sent_seq;
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int main(int argc, char *argv[])
{

	unsigned nb_ports;
	uint16_t portid;

    if (argc == 3) {
        flow_num = (int) atoi(argv[1]);
        flow_size =  (int) atoi(argv[2]);
    } else {
        printf( "usage: ./lab1-client <flow_num> <flow_size>\n");
        return 1;
    }

    NUM_PING = flow_size / payload_len;

	/* Initializion the Environment Abstraction Layer (EAL). 8< */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	
	argc -= ret;
	argv += ret;

    nb_ports = rte_eth_dev_count_avail();
	/* Allocates mempool to hold the mbufs. 8< */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
										MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	/* >8 End of allocating mempool to hold mbuf. */

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initializing all ports. 8< */
	RTE_ETH_FOREACH_DEV(portid)
        if (portid == 1 && port_init(portid, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n",
                    portid);
    sleep(5);
	
	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the main core only. Called on single lcore. 8< */
	lcore_main();
	
	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}

size_t get_appl_data() {
    return 100;
}