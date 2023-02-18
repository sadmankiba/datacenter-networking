/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
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

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define N_PKT_WRT 100

struct rte_mbuf * construct_pkt(size_t, uint32_t);
size_t get_appl_data();

uint32_t NUM_PING;

/* Define the mempool globally */
struct rte_mempool *mbuf_pool = NULL;
static struct rte_ether_addr my_eth;
uint16_t eth_port_id = 1;
static uint32_t seconds = 1;

size_t window_len = 10;
size_t buf_size = 500;

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
 * Parse a complete TCP packet. Read src IP, dst IP, payload 
 * and payload length from mbuf packet.
 * 
 * @return
 *   - (0) if not a TCP packet
 *   - (+ve integer) denoting flow id
 */
static int parse_packet(struct sockaddr_in *src,
                        struct sockaddr_in *dst,
                        uint64_t *recv_ack,
                        void **payload,
                        size_t *payload_len,
                        struct rte_mbuf *pkt)
{
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
    in_addr_t ipv4_src_addr = ip_hdr->src_addr;
    in_addr_t ipv4_dst_addr = ip_hdr->dst_addr;

    if (IPPROTO_TCP != ip_hdr->next_proto_id) {
        return 0;
    }
    
    src->sin_addr.s_addr = ipv4_src_addr;
    dst->sin_addr.s_addr = ipv4_dst_addr;
    
    /* Parse tcp header */
    struct rte_tcp_hdr * const tcp_hdr = (struct rte_tcp_hdr *)(p);
    p += sizeof(*tcp_hdr);
    header += sizeof(*tcp_hdr);

    in_port_t tcp_src_port = tcp_hdr->src_port;
    in_port_t tcp_dst_port = tcp_hdr->dst_port;
    int ret = 0;
	
	uint16_t p1 = rte_cpu_to_be_16(5001);
	uint16_t p2 = rte_cpu_to_be_16(5002);
	uint16_t p3 = rte_cpu_to_be_16(5003);
	uint16_t p4 = rte_cpu_to_be_16(5004);
	
	if (tcp_hdr->dst_port ==  p1)
	{
		ret = 1;
	}
	if (tcp_hdr->dst_port ==  p2)
	{
		ret = 2;
	}
	if (tcp_hdr->dst_port ==  p3)
	{
		ret = 3;
	}
	if (tcp_hdr->dst_port ==  p4)
	{
		ret = 4;
	}

    src->sin_port = tcp_src_port;
    dst->sin_port = tcp_dst_port;
    
    src->sin_family = AF_INET;
    dst->sin_family = AF_INET;
    
    *recv_ack = tcp_hdr->recv_ack;
    *payload_len = pkt->pkt_len - header;
    *payload = (void *)p;
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


/* Send and receive packet on an Ethernet port. */
void lcore_main()
{
    /* Data structures for sending */
    struct rte_mbuf *pkts_rcvd[BURST_SIZE];
    struct rte_mbuf *pkt;
    uint16_t nb_rx;
    uint64_t n_pkts_acked = 0;
    size_t header_size;
    
    int outstanding[flow_num];
    uint16_t seq[flow_num];
    size_t flow_id = 0;
    for(size_t i = 0; i < flow_num; i++)
    {
        outstanding[i] = 0;
        seq[i] = 0;
    } 
    /* Data structures for receiving */
    struct sockaddr_in src, dst;
    void *payload = NULL;
    size_t recv_pld_len;

    /* Timing variables */
    uint64_t lat_cum_us = 0;
    uint64_t flow_start_time = raw_time_ns();
    uint64_t lat_ns;

    /* Sliding window variables */
    struct rte_mbuf *buf[buf_size];
    uint64_t last_pkt_acked = 0; /* 1-indexed */
    uint64_t last_pkt_sent = 0;
    uint64_t last_pkt_written = 0;
    uint64_t recv_ack = 0;
    size_t n_empty_buf = buf_size;
    uint64_t retran[buf_size];
    uint16_t retran_last_idx = 0;
    uint16_t window = window_len;
    uint64_t time_sent[window_len];

    for(int i = 0; i < buf_size; i++)
        retran[i] = -1;
    
    debug("To send %u packets\n", NUM_PING);
    /* Repeatedly construct packet, retransmit, send data, recv ack. */
    while (last_pkt_acked < NUM_PING) {
        debug("---New iter---\nlast_pkt_acked: %lu, last_pkt_sent: %lu, last_pkt_written: %lu\n",
            last_pkt_acked, last_pkt_sent, last_pkt_written);
        size_t n_new_pkt = NUM_PING - last_pkt_written < N_PKT_WRT? 
            NUM_PING - last_pkt_written: N_PKT_WRT; 
        for(int i = 0; i < n_new_pkt && n_empty_buf > 0; i++) {
            pkt = construct_pkt(flow_id, last_pkt_written + 1);
            buf[(last_pkt_written) % buf_size] = pkt;
            last_pkt_written += 1;
            n_empty_buf--;
        }
        /* Retransmit unacked data */
        for (int i = 0; i < buf_size; i++) {
            uint16_t retran_idx = (retran_last_idx + i) % buf_size;
            if(retran[retran_idx] != -1) break;
            //send
        }

        /* Send data */
        while((last_pkt_sent - last_pkt_acked < window) 
            && (last_pkt_sent < last_pkt_written)) {
            rte_eth_tx_burst(1, 0, &(buf[(last_pkt_sent) % buf_size]), 1);
            time_sent[last_pkt_sent % window_len] = raw_time_ns();
            last_pkt_sent++;
            debug("Sent pkt seq %lu\n", last_pkt_sent);
        }

        /* Recv data */
        while(true) {
            nb_rx = rte_eth_rx_burst(eth_port_id, 0, pkts_rcvd, BURST_SIZE);
            uint64_t time_recvd = raw_time_ns();
            debug("Received %u packets\n", nb_rx);
            if(nb_rx == 0) break;
            
            for (int i = 0; i < nb_rx; i++) {
                int p = parse_packet(&src, &dst, &recv_ack, &payload, &recv_pld_len, pkts_rcvd[i]);
                if (p != 0) {
                    if (recv_ack > last_pkt_acked) {
                        debug("Received ack %lu\n", recv_ack);
                        n_empty_buf += (recv_ack - last_pkt_acked);
                        last_pkt_acked = recv_ack;
                        n_pkts_acked++;
                        lat_ns = time_recvd - time_sent[recv_ack % window_len];
                        lat_cum_us += (uint64_t) (lat_ns * 1.0 / 1000);
                    }
                }
                rte_pktmbuf_free(pkts_rcvd[i]);
            }
        }
        debug("n_pkts_acked %lu, lat_cum_us %lu, n_empty_buf %lu\n",
            n_pkts_acked, lat_cum_us, n_empty_buf);
    }

    /* latency in us */
    printf("%f, ", (1.0 * lat_cum_us) / n_pkts_acked); 
    
    /* Bw in Gbps */
    printf("%f", (1.0 * n_pkts_acked * (header_size + payload_len) * 8) / time_now_ns(flow_start_time));
}

struct rte_mbuf * construct_pkt(size_t flow_id, uint32_t sent_seq) {
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
    uint16_t srcp = 5001 + flow_id;
    uint16_t dstp = 5001 + flow_id;
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