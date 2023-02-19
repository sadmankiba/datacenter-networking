/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include "debug.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define ETH_PORT_ID 1

#define NUM_MBUFS 65535
#define MBUF_CACHE_SIZE 512
#define BUF_SIZE 128
#define BURST_SIZE 1024
#define N_PKT_READ 1024
#define STOP_FLOW_ID 100
#define MAX_FLOW_NUM 8


struct rte_mbuf * construct_ack(struct rte_mbuf *, uint8_t, uint32_t, uint32_t);
uint32_t get_seq(struct rte_mbuf *);

struct rte_mempool *mbuf_pool = NULL;

static struct rte_ether_addr my_eth;

int ack_len = 10;

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

void print_mbuf(struct rte_mbuf *pkt) {
	rte_pktmbuf_dump(stdout, pkt, pkt->pkt_len);
}

/*
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
/* >8 End of main functional part of port initialization. */

/* 
 * Parse an Ethernet packet to check if it contains TCP packet. 
 * 
 * @return
 *   - (0) if not a TCP packet
 *   - (+ve integer) denoting flow id
 */
uint8_t verify_pkt(struct rte_mbuf *pkt)
{
    /*
	 * Packet layout order is (from outside -> in):
     * ether_hdr | ipv4_hdr | tcp_hdr | payload
	 */

    uint8_t *p = rte_pktmbuf_mtod(pkt, uint8_t *);
    size_t header_size = 0;

    /* Parse Ethernet header and Validate dst MAC */
    struct rte_ether_hdr * const eth_hdr = (struct rte_ether_hdr *)(p);
    p += sizeof(*eth_hdr);
    header_size += sizeof(*eth_hdr);
    uint16_t eth_type = ntohs(eth_hdr->ether_type);
    struct rte_ether_addr mac_addr = {};
	rte_eth_macaddr_get(ETH_PORT_ID, &mac_addr);
    
	
	if (!rte_is_same_ether_addr(&mac_addr, &eth_hdr->dst_addr) ) {
        return 0;
    }
    if (RTE_ETHER_TYPE_IPV4 != eth_type) {
        return 0;
    }

    /* Parse IP header and validate Type = TCP */
    struct rte_ipv4_hdr *const ip_hdr = (struct rte_ipv4_hdr *)(p);
    p += sizeof(*ip_hdr);
    header_size += sizeof(*ip_hdr);
    in_addr_t ipv4_src_addr = ip_hdr->src_addr;
    in_addr_t ipv4_dst_addr = ip_hdr->dst_addr;

    if (IPPROTO_TCP != ip_hdr->next_proto_id) {
        return 0;
    }

    /* Parse TCP Header */
    struct rte_tcp_hdr * const tcp_hdr = (struct rte_tcp_hdr *)(p);
    p += sizeof(*tcp_hdr);
    header_size += sizeof(*tcp_hdr);
	
	uint8_t ret = 0;
	uint16_t p1 = rte_cpu_to_be_16(5001);
	uint16_t p2 = rte_cpu_to_be_16(5002);
	uint16_t p3 = rte_cpu_to_be_16(5003);
	uint16_t p4 = rte_cpu_to_be_16(5004);
	uint16_t p5 = rte_cpu_to_be_16(5005);
	uint16_t p6 = rte_cpu_to_be_16(5006);
	uint16_t p7 = rte_cpu_to_be_16(5007);
	uint16_t p8 = rte_cpu_to_be_16(5008);
	uint16_t ps = rte_cpu_to_be_16(5001 + STOP_FLOW_ID);

	if (tcp_hdr->dst_port == p1) ret = 1;
	if (tcp_hdr->dst_port == p2) ret = 2;
	if (tcp_hdr->dst_port == p3) ret = 3;
	if (tcp_hdr->dst_port == p4) ret = 4;
	if (tcp_hdr->dst_port == p5) ret = 5;
	if (tcp_hdr->dst_port == p6) ret = 6;
	if (tcp_hdr->dst_port == p7) ret = 7;
	if (tcp_hdr->dst_port == p8) ret = 8;
	if (tcp_hdr->dst_port == ps) ret = STOP_FLOW_ID + 1;
	debug("Parsed packet of flow %u, seq %u\n", 
			ret - 1, tcp_hdr->sent_seq);
		
	return ret;
}

/* Receive packets on Ethernet port and reply ack packet. */
void lcore_main(void)
{
	uint16_t port;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	RTE_ETH_FOREACH_DEV(port)
		if (rte_eth_dev_socket_id(port) >= 0 &&
			rte_eth_dev_socket_id(port) !=
				(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
				"polling thread.\n\tPerformance will "
				"not be optimal.\n",
				port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
		   rte_lcore_id());

	/* Receive data structure */
	struct rte_mbuf *rcvd_pkts[BURST_SIZE];
	struct rte_mbuf *pkt;
	struct rte_mbuf *ack;
	uint32_t n_rcvd;
	
	/* Sliding window receiving variables */
	uint32_t last_pkt_read[MAX_FLOW_NUM] = {0}; /* 1-indexed */
	uint32_t nxt_pkt_expcd[MAX_FLOW_NUM];
	uint32_t last_pkt_recvd[MAX_FLOW_NUM] = {0};
	uint32_t n_buf_pkts[MAX_FLOW_NUM] = {0};
	struct rte_mbuf *buf[MAX_FLOW_NUM][BUF_SIZE];
	uint32_t seq;
	uint8_t pkts_to_cnsm[MAX_FLOW_NUM];
	struct rte_mbuf *snd_pkts[MAX_FLOW_NUM];

	for (uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
		nxt_pkt_expcd[fid] = 1;
		for(uint16_t i = 0; i < BUF_SIZE; i++) {
			buf[fid][i] = NULL;
		}
	}
	
	/* 
	 * In each iter, consume buffer, get a burst of RX packets 
	 * and reply an ack if required 
	 */ 
	for (;;) {
		/* Read buf */
		for (uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
			pkts_to_cnsm[fid] = (n_buf_pkts[fid] < N_PKT_READ)? n_buf_pkts[fid]: N_PKT_READ;
			for (uint32_t i = 0; i < pkts_to_cnsm[fid]; i++) {
				// rte_pktmbuf_free(buf[fid][(last_pkt_read[fid] + i) % BUF_SIZE]);
				buf[fid][(last_pkt_read[fid] + i) % BUF_SIZE] = NULL;
				n_buf_pkts[fid]--;
			}

			last_pkt_read[fid] += pkts_to_cnsm[fid];
		}
		
		bool rcvd[MAX_FLOW_NUM] = {0};
		uint8_t rfid;
		RTE_ETH_FOREACH_DEV(port) {
			if (port != ETH_PORT_ID)
				continue;
			
			n_rcvd = rte_eth_rx_burst(port, 0, rcvd_pkts, BURST_SIZE);

			if (unlikely(n_rcvd == 0))
				continue;

			/* Add each rcvd pkt to buffer */
			uint8_t resp;
			for(uint32_t i = 0; i < n_rcvd; i++) {
				pkt = rcvd_pkts[i];
				resp = verify_pkt(pkt);
				if(resp == 0) {
					rte_pktmbuf_free(pkt);
					continue;
				} 
				rfid = resp - 1;
				if (rfid == STOP_FLOW_ID) break;
				
				rcvd[rfid] = true;
				seq = get_seq(pkt);
				if (seq >= nxt_pkt_expcd[rfid] && buf[rfid][(seq - 1) % BUF_SIZE] == NULL) {
					buf[rfid][(seq - 1) % BUF_SIZE] = pkt;
					n_buf_pkts[rfid]++;
				}
				if (seq > last_pkt_recvd[rfid])
					last_pkt_recvd[rfid] = seq;
			
			}
		}
		if (n_rcvd == 0) continue;
		if (rfid == STOP_FLOW_ID) break;

		int new_seqd_pkt[MAX_FLOW_NUM] = {0};
		uint8_t n_snd = 0;
		for (uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
			if(rcvd[fid] == false) continue;
			
			/* Find the highest seq packet rcvd */
			while(true) {
				pkt = buf[fid][(nxt_pkt_expcd[fid] - 1 + new_seqd_pkt[fid]) % BUF_SIZE];
				if (pkt != NULL && get_seq(pkt) == (nxt_pkt_expcd[fid] + new_seqd_pkt[fid])
					&& new_seqd_pkt[fid] < BUF_SIZE) 
					new_seqd_pkt[fid]++;
				else break;
			}
			
			nxt_pkt_expcd[fid] += new_seqd_pkt[fid];

			/* Construct an ack packet for (nxt_pkt_expcd - 1). */
			ack = construct_ack(rcvd_pkts[0], fid,
				nxt_pkt_expcd[fid] - 1, BUF_SIZE - n_buf_pkts[fid]);	
			
			if(ack == NULL) continue;
			/* Send back ack. */
			snd_pkts[n_snd++] = ack;
			debug("Will send flow %u, ack %u\n", fid, nxt_pkt_expcd[fid] - 1);
		}
		for (uint32_t i = 0; i < n_rcvd; i++) {	
			rte_pktmbuf_free(rcvd_pkts[i]);
		}
		
		rte_eth_tx_burst(ETH_PORT_ID, 0, snd_pkts, n_snd);
		for(uint16_t i = 0; i < n_snd; i++)
			rte_pktmbuf_free(snd_pkts[i]);
		
		debug("last_pkt_read: "); 
        for(uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
            debug("%u, ", last_pkt_read[fid]);
        }
		debug("nxt_pkt_expcd: "); 
        for(uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
            debug("%u, ", nxt_pkt_expcd[fid]);
        }
		debug("last_pkt_recvd: "); 
        for(uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
            debug("%u, ", last_pkt_recvd[fid]);
        }
		debug("n_buf_pkts: "); 
        for(uint8_t fid = 0; fid < MAX_FLOW_NUM; fid++) {
            debug("%u, ", n_buf_pkts[fid]);
        }
		debug("\n");
	}
}

/* Construct Ack packet for a received packet using input ack num and window size. */
struct rte_mbuf * construct_ack(struct rte_mbuf *pkt, uint8_t fid, uint32_t ack_num, uint32_t wnd) {
	struct rte_ether_hdr *eth_h;
	struct rte_ipv4_hdr *ip_h;
	struct rte_tcp_hdr *tcp_h;

	struct rte_mbuf *ack;
	struct rte_ether_hdr *eth_h_ack;
	struct rte_ipv4_hdr *ip_h_ack;
	struct rte_tcp_hdr *tcp_h_ack;

	eth_h = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	ip_h = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *,
									sizeof(struct rte_ether_hdr));
	tcp_h = rte_pktmbuf_mtod_offset(pkt, struct rte_tcp_hdr *,
									sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) );

	/* Allocate ack packet */
	ack = rte_pktmbuf_alloc(mbuf_pool);
	if (ack == NULL) {
		printf("Error allocating tx mbuf\n");
		return NULL;
	}
	size_t header_size = 0;

	uint8_t *ptr = rte_pktmbuf_mtod(ack, uint8_t *);
	
	/* add in an ethernet header */
	eth_h_ack = (struct rte_ether_hdr *)ptr;
	rte_ether_addr_copy(&my_eth, &eth_h_ack->src_addr);
	rte_ether_addr_copy(&eth_h->src_addr, &eth_h_ack->dst_addr);
	eth_h_ack->ether_type = rte_be_to_cpu_16(RTE_ETHER_TYPE_IPV4);
	ptr += sizeof(*eth_h_ack);
	header_size += sizeof(*eth_h_ack);

	/* add in ipv4 header */
	ip_h_ack = (struct rte_ipv4_hdr *)ptr;
	ip_h_ack->version_ihl = 0x45;
	ip_h_ack->type_of_service = 0x0;
	ip_h_ack->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr) + ack_len);
	ip_h_ack->packet_id = rte_cpu_to_be_16(1);
	ip_h_ack->fragment_offset = 0;
	ip_h_ack->time_to_live = 64;
	ip_h_ack->next_proto_id = IPPROTO_TCP;
	ip_h_ack->src_addr = ip_h->dst_addr;
	ip_h_ack->dst_addr = ip_h->src_addr;

	uint32_t ipv4_checksum = wrapsum(checksum((unsigned char *)ip_h_ack, sizeof(struct rte_ipv4_hdr), 0));
	ip_h_ack->hdr_checksum = rte_cpu_to_be_32(ipv4_checksum);
	header_size += sizeof(*ip_h_ack);
	ptr += sizeof(*ip_h_ack);
	
	/* add in TCP hdr */
	tcp_h_ack = (struct rte_tcp_hdr *)ptr;
	tcp_h_ack->src_port = rte_cpu_to_be_16(5001 + fid);
	tcp_h_ack->dst_port = rte_cpu_to_be_16(5001 + fid);
	tcp_h_ack->sent_seq = 0;
	tcp_h_ack->recv_ack = ack_num;
	tcp_h_ack->data_off = 0;
	tcp_h_ack->tcp_flags = 0;
	tcp_h_ack->rx_win = wnd;
	tcp_h_ack->cksum = 0;
	tcp_h_ack->tcp_urp = 0;
	
	/* set the payload */
	header_size += sizeof(*tcp_h_ack);
	ptr += sizeof(*tcp_h_ack);
	memset(ptr, 'a', ack_len);

	ack->l2_len = RTE_ETHER_HDR_LEN;
	ack->l3_len = sizeof(struct rte_ipv4_hdr);
	ack->data_len = header_size + ack_len;
	ack->pkt_len = header_size + ack_len;
	ack->nb_segs = 1;
	debug("Constructed ack #%u\n", tcp_h_ack->recv_ack);

	return ack;
}

uint32_t get_seq(struct rte_mbuf *pkt) {
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
	// struct rte_mempool *mbuf_pool;
	unsigned nb_ports = 1;
	uint16_t portid;
	
	/* Initializion the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	nb_ports = rte_eth_dev_count_avail();
	/* Allocates mempool to hold the mbufs. 8< */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
										MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize port 1 */
	RTE_ETH_FOREACH_DEV(portid) {
		printf("%u\n", portid);
		if (portid == 1 && port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n",
					portid);
	}

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the main core only. Called on single lcore. 8< */
	lcore_main();

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
