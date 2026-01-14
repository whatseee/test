#ifdef TAP_INTERFACE

#include "../include/scps.h"
#include "../include/dpdkinit.h"
#include "scps_ip.h"
#include "scpsudp.h"
#include "tp_debug.h"  // Included for LOG_PACKET in case DEBUG_TCPTRACE is defined.
#include "tap.h"
#include "other_proto_handler.h"
#include <syslog.h>
extern struct rte_ring *ring;

#ifdef __FreeBSD__
#define __ISBSDISH__
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
#define __ISBSDISH__
#endif /* __NetBSD__ */
#ifdef __OpenBSD__
#define __ISBSDISH__
#endif /* __OpenBSD__ */

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: tap.c,v $ -- $Revision: 1.13 $\n";
#endif

extern struct msghdr out_msg;
extern struct _ll_queue_element *in_data;

extern int scps_udp_port;
extern int scps_udp_port1;
extern int errno;

udp_Header *up;

#ifdef GATEWAY
#include "rs_config.h"
extern GW_ifs gw_ifs;
extern struct _interface *sock_interface;
extern struct _interface *divert_interface;
extern	struct lcore_queue_conf *qconf;

#ifdef GATEWAY_DUAL_INTERFACE
extern int special_port_number;
extern uint32_t special_ip_addr;
#endif /* GATEWAY_DUAL_INTERFACE */
#endif /* GATEWAY */

extern uint32_t l2fwd_dst_ports[RTE_MAX_ETHPORTS];

extern void err_dump ();

#ifdef LINUX 
int
tap_open (dev)
char *dev;

{
    struct ifreq ifr;
    int fd, err;
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0){
        return fd;
    }
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        return err;
    }

  if (fcntl (fd, F_SETFL, O_NDELAY) > 0)
    err_dump ("fcntl problem");

  if (fcntl (fd, F_SETOWN, getpid ()) > 0)
    err_dump ("fcntl problem with F_SETOWN");

    if(dev){
        strncpy(dev, ifr.ifr_name,IFNAMSIZ);
    }
    return fd;
}
#endif /* LINUX */

#if defined(__FreeBSD__) || defined(__NetBSD__)
/*
 * Allocate Ether TAP device, returns opened fd.
 * Stores dev name in the first arg(must be large enough).
 */

int tap_open(char *dev)
{
    char tapname[14];
    int i, fd;

	sprintf(tapname, "/dev/%s", dev);

/* Open device */
	if( (fd=open(tapname, O_RDWR)) > 0 ) {

		if (fcntl (fd, F_SETFL, O_NDELAY) > 0)
			err_dump ("fcntl problem");

		if (fcntl (fd, F_SETFL, FIONBIO) > 0)
			err_dump ("fcntl problem");

		if (fcntl (fd, F_SETOWN, getpid ()) > 0)
			err_dump ("fcntl problem with F_SETOWN");

		return fd;
	} else {
		return -1;
	}
}

int tap_close(int fd, char *dev)
{
    return close(fd);
}

/* Write frames to TAP device */
int tap_write(int fd, char *buf, int len)
{
    return write(fd, buf, len);
}

/* Read frames from TAP device */
int tap_read(int fd, char *buf, int len)
{
    return read(fd, buf, len);
}

#endif /* __FreeBSD__ || __NetBSD__ */

int ll_tap_qk_send  (tun_fd, data, len)
int tun_fd;
unsigned char *data;
int len;
{
    	int rc, i;

#ifdef DISPLAY_PKT
{
    printf ("%s %d ll_tap_quick_send\n", __FILE__, __LINE__);
    for (i = 0; i < len; i++)
    {
        printf ("%2x ", (unsigned char) (0x0ff & (data[i])));
        if ((i +1) % 16 == 0)
          printf ("\n");
    }
    printf ("\n");
}
#endif /* DISPLAY_PKT */

	struct rte_mbuf *send_m = rte_pktmbuf_alloc(l2fwd_pktmbuf_pool);

	if (send_m == NULL)
		printf("Alloc pktmbuf failed!!!\n");
	//printf("Alloc pktmbuf succeed!!!\n");
	memcpy(rte_pktmbuf_mtod(send_m, void *), data, len);
	send_m->nb_segs = 1;
	send_m->next = NULL;
	send_m->pkt_len = (uint16_t)len;
	send_m->data_len = (uint16_t)len;
	//rte_pktmbuf_dump(stdout, m, 0);
	
	rc = rte_eth_tx_burst(tun_fd, 0, &send_m, 1);	
	//printf("ll_tap_quick_send: dstport:%d, rc:%d\n\n", tun_fd, rc);
   	if (unlikely(rc < 1)) 
	{	
		printf("dpdk drop one packet!\n");
		rte_pktmbuf_free(send_m);		
	}
	else
	{
		;//rte_pktmbuf_free(send_m);
	}
	//rc = write (tun_fd, data, len);

	return (rc);
}


int ll_tap_send  (struct _interface *interface, 
						uint32_t remote_internet_addr,
            		int protocol, 
						int data_len, 
						struct msghdr *my_msg,
            		route *a_route, 
						scps_np_rqts *rqts)
{
	int sock = 0;
	struct sockaddr_in remote_addr;
	//unsigned char linear_buffer[MAX_MTU];
	unsigned char *linear_buffer = my_malloc(1514);
	struct _interface *interface_t = NULL;
    interface_t = scheduler.interface;
	int length;
	int size = 0;
	int rval;
	int i;
	tp_Socket * tmpsock = (tp_Socket *)rqts->sock;
	memset((char *) &remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(scps_udp_port);
	remote_addr.sin_addr.s_addr=remote_internet_addr;
	my_msg->msg_iov[0].iov_len = 0;

	sock = rqts->recv_tap_if; 

#ifdef GATEWAY_DUAL_INTERFACE
   	if (interface == sock_interface) 
	{
		my_msg->msg_name = (caddr_t) &remote_addr;
   		my_msg->msg_namelen = sizeof(remote_addr);

   		// XXX Toss this and replace write with a writev().
		if(0 != rqts->TPID)  //vlan
		{
  			memcpy (&(linear_buffer [0]), &(rqts->dst_mac_addr [0]), MAC_ADDR_SIZE);
  			memcpy (&(linear_buffer [MAC_ADDR_SIZE]), &(rqts->src_mac_addr [0]), MAC_ADDR_SIZE);

  			linear_buffer [START_OF_FRAME_TYPE + 0] = (rqts->TPID & (0xff00)) >> 8; 
  			linear_buffer [START_OF_FRAME_TYPE + 1] = (rqts->TPID & (0x00ff));
  			linear_buffer [START_OF_FRAME_TYPE + 2] = (rqts->TCI & (0xff00)) >> 8; 
  			linear_buffer [START_OF_FRAME_TYPE + 3] = (rqts->TCI & (0x00ff));
  			linear_buffer [START_OF_FRAME_TYPE + 4] = (rqts->frame_type & (0xff00)) >> 8; 
  			linear_buffer [START_OF_FRAME_TYPE + 5] = (rqts->frame_type & (0x00ff));

			//Copy and modify IP header
			memcpy(&linear_buffer[SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG], my_msg->msg_iov[1].iov_base, 20/*Do not copy IP options!*/);
			struct ipv4_s *ipv4_header = (struct ipv4_s*)(&linear_buffer[SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG]);
			ipv4_header->length = htons(28 + ntohs(ipv4_header->length));
			ipv4_header->ttlProtocol = htons( (ntohs(ipv4_header->ttlProtocol)&0xff00)+SCPSUDP );
			ipv4_header->checksum = 0;
			ipv4_header->checksum = ~checksum( (word*)ipv4_header, ( (ntohs(ipv4_header->vht)>>6) & 0x3c) );

			//Generate UDP header
			udp_Header *udp_header = (udp_Header*)(&linear_buffer[SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG+20]);
			udp_header->srcPort = htons(scps_udp_port);
			udp_header->dstPort = htons(scps_udp_port1);
			udp_header->len = htons(ntohs(ipv4_header->length) - 20);
			udp_header->checksum = 0;

   			for (i = 0,length=0,size=SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG+28; i < my_msg->msg_iovlen; i++) 
			{
   				length = my_msg->msg_iov[i].iov_len;
      			memcpy(&linear_buffer[size], my_msg->msg_iov[i].iov_base, length);
      			size += length;
    			}
		}
		else
		{
  			memcpy (&(linear_buffer [0]), &(rqts->dst_mac_addr [0]), MAC_ADDR_SIZE);
  			memcpy (&(linear_buffer [MAC_ADDR_SIZE]), &(rqts->src_mac_addr [0]), MAC_ADDR_SIZE);
  			linear_buffer [START_OF_FRAME_TYPE] = (rqts->frame_type & (0xff00)) >> 8; 
  			linear_buffer [START_OF_FRAME_TYPE +1] = (rqts->frame_type & (0x00ff));

			//Copy and modify IP header
			memcpy(&linear_buffer[SIZE_OF_ETHER_PART], my_msg->msg_iov[1].iov_base, 20/*Do not copy IP options!*/);
			struct ipv4_s *ipv4_header = (struct ipv4_s*)(&linear_buffer[SIZE_OF_ETHER_PART]);
			ipv4_header->length = htons(28 + ntohs(ipv4_header->length));
			ipv4_header->ttlProtocol = htons( (ntohs(ipv4_header->ttlProtocol)&0xff00)+SCPSUDP );
			ipv4_header->checksum = 0;
			ipv4_header->checksum = ~checksum( (word*)ipv4_header, ( (ntohs(ipv4_header->vht)>>6) & 0x3c) );

			//Generate UDP header
			udp_Header *udp_header = (udp_Header*)(&linear_buffer[SIZE_OF_ETHER_PART+20]);
			udp_header->srcPort = htons(scps_udp_port);
			udp_header->dstPort = htons(scps_udp_port1);
			udp_header->len = htons(ntohs(ipv4_header->length) - 20);
			udp_header->checksum = 0;

   			for (i = 0,length=0,size=SIZE_OF_ETHER_PART+28; i < my_msg->msg_iovlen; i++) 
			{
   				length = my_msg->msg_iov[i].iov_len;
      			memcpy(&linear_buffer[size], my_msg->msg_iov[i].iov_base, length);
      			size += length;
    			}
		}

#ifdef DISPLAY_PKT
	   	printf ("%s %d ll_tap_send\n", __FILE__, __LINE__);
	   	for (i = 0; i < size; i++)
			{
	   		printf ("%2x ", (unsigned char) (0x0ff & (linear_buffer[i])));
	      	if ((i +1) % 16 == 0)
	      		printf ("\n");
			}
	   	printf ("\n\n");
#endif /* DISPLAY_PKT */

   		rval=write(sock,linear_buffer,size);
   
		return rval;
  	}
#endif /* GATEWAY_DUAL_INTERFACE */

   	my_msg->msg_name = (caddr_t) &remote_addr;
   	my_msg->msg_namelen = sizeof(remote_addr);

   	// XXX Toss this and replace write with a writev().
	if(0 != rqts->TPID)  //vlan
	{
  		memcpy (&(linear_buffer [0]), &(rqts->dst_mac_addr [0]), MAC_ADDR_SIZE);
  		memcpy (&(linear_buffer [MAC_ADDR_SIZE]), &(rqts->src_mac_addr [0]), MAC_ADDR_SIZE);

  		linear_buffer [START_OF_FRAME_TYPE + 0] = (rqts->TPID & (0xff00)) >> 8; 
  		linear_buffer [START_OF_FRAME_TYPE + 1] = (rqts->TPID & (0x00ff));
  		linear_buffer [START_OF_FRAME_TYPE + 2] = (rqts->TCI & (0xff00)) >> 8; 
  		linear_buffer [START_OF_FRAME_TYPE + 3] = (rqts->TCI & (0x00ff));
  		linear_buffer [START_OF_FRAME_TYPE + 4] = (rqts->frame_type & (0xff00)) >> 8; 
  		linear_buffer [START_OF_FRAME_TYPE + 5] = (rqts->frame_type & (0x00ff));

		if(rqts->mpls_depth > 0)            //MPLS
		{
			for(i=0; i<rqts->mpls_depth; i++)
			{
				memcpy(&linear_buffer[START_OF_FRAME_TYPE+6+i*SIZE_OF_MPLS_TAG], (unsigned char*)&(rqts->mpls[i]), SIZE_OF_MPLS_TAG);
			}
		}
   		for (i = 0,length=0,size=SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG+rqts->mpls_depth*SIZE_OF_MPLS_TAG; i < my_msg->msg_iovlen; i++) 
		{
   			length = my_msg->msg_iov[i].iov_len;
			memcpy(&linear_buffer[size], my_msg->msg_iov[i].iov_base, length);
			size += length;
    		}
	}
	else
	{
  		memcpy (&(linear_buffer [0]), &(rqts->dst_mac_addr [0]), MAC_ADDR_SIZE);
  		memcpy (&(linear_buffer [MAC_ADDR_SIZE]), &(rqts->src_mac_addr [0]), MAC_ADDR_SIZE);
  		linear_buffer [START_OF_FRAME_TYPE] = (rqts->frame_type & (0xff00)) >> 8; 
  		linear_buffer [START_OF_FRAME_TYPE +1] = (rqts->frame_type & (0x00ff));

		if(rqts->mpls_depth > 0)		//MPLS
		{
			for(i=0; i<rqts->mpls_depth; i++)
			{
				memcpy(&linear_buffer[START_OF_FRAME_TYPE+2+i*SIZE_OF_MPLS_TAG], (unsigned char*)&(rqts->mpls[i]), SIZE_OF_MPLS_TAG);
			}
		}

   		for (i = 0,length=0,size=SIZE_OF_ETHER_PART+rqts->mpls_depth*SIZE_OF_MPLS_TAG; i < my_msg->msg_iovlen; i++)	 
		{
   			length = my_msg->msg_iov[i].iov_len;
			memcpy(&linear_buffer[size], my_msg->msg_iov[i].iov_base, length);
			size += length;
    		}
		if(rqts->mpls_depth == 0)
		{
			linear_buffer [START_OF_FRAME_TYPE] = ((0x0800) & (0xff00)) >> 8; 
  			linear_buffer [START_OF_FRAME_TYPE +1] = ((0x0800) & (0x00ff));
		}
	}

#ifdef DISPLAY_PKT
   	printf ("%s %d ll_tap_send\n", __FILE__, __LINE__);
   	for (i = 0; i < size; i++)
        {
   		printf ("%2x ", (unsigned char) (0x0ff & (linear_buffer[i])));
		fflush(stdout);
     		if ((i +1) % 16 == 0)
		{
      		printf ("\n");
			fflush(stdout);
		}
        }
   	printf ("\n\n");
#endif /* DISPLAY_PKT */
// rval = size;
// uint32_t next_send_time = 0;
// uint32_t tp_now = get_time_us(); 
// 	addNode(linear_buffer, next_send_time, sock,  size, tp_now);
   	//rval=write(sock,linear_buffer,size);

	uint32_t next_send_time = 0;
	uint32_t tp_now = get_time_us(); 
	uint32_t next_to_send_time = 0;
	if(rqts->pre_send_time == 0)
	{
		rqts->pre_send_time = tp_now;
		
	}
	if(interface_t->tap_b_fd == sock)
	{
		next_to_send_time = (1448 * 1.0) /rqts->fpacing_rate;
		//next_to_send_time = 100;
		if(next_to_send_time > 300)
		{
			next_to_send_time = 300;
		}
		if(next_to_send_time < 10)
		{
			next_to_send_time = 10;
		}
		next_send_time = rqts->pre_send_time + next_to_send_time;
		//next_send_time = tp_now;
	}
	else
	{
		next_send_time = tp_now;
	}
	
	if(tp_now > next_send_time)
	{
		next_send_time = tp_now;
	}	
	if(sock == interface_t->tap_a_fd && size >= 1000)
	{
		if(tmpsock)
		{
			tmpsock->remote_flag = 2;
			if(tmpsock->peer_socket)
			{
				tmpsock->peer_socket->remote_flag = 2;
			}
			else
			{
				printf("tmdsock -> peer is NULL\n");
			}
		}	
		else
		{
			printf("tmdsock is NULL\n");
		}
		//remote_flag = 2;
		//printf("ref = %d\n",remote_flag);
	}

	if(sock == interface_t->tap_b_fd && size >= 1000)
	{
		if(tmpsock)
		{
			tmpsock->is_bbr_sk = 10000;
			tmpsock->remote_flag = 0;
			if(tmpsock->peer_socket)
			{
				tmpsock->peer_socket->remote_flag = 0;
			}
			else
			{
				printf("tmdsock -> peer is NULL\n");
			}
		}
		else
		{
			printf("tmdsock is NULL\n");
		}
	}
	rval = size;
	if(interface_t->tap_b_fd == sock)
	{
		Data *d = my_malloc(sizeof(Data));
		d->data = linear_buffer;
		d->size = size;
		d->sock = sock;
		d->time = next_send_time;
		d->timesample = tp_now;

		while (rte_ring_enqueue(ring, (void *)d) < 0) {
            //printf("Failed to enqueue element %d\n", i);
        }

		//addNode(linear_buffer, next_send_time, sock,  size, tp_now);
		rqts->pre_send_time = next_send_time;
	}
	else
	{
#if 1
		struct rte_mbuf *send_m = rte_pktmbuf_alloc(l2fwd_pktmbuf_pool);
		if (send_m == NULL)
			printf("Alloc pktmbuf failed!!!\n");

		memcpy(rte_pktmbuf_mtod(send_m, void *), linear_buffer, size);
		send_m->nb_segs = 1;
		send_m->next = NULL;
		send_m->pkt_len = (uint16_t)size;
		send_m->data_len = (uint16_t)size;


		//printf("tap_send: dstport:%d\n", sock);
		int rval = rte_eth_tx_burst(sock, 0, &send_m, 1);	
		if (unlikely(rval < 1)) 
		{	
			printf("dpdk drop one packet!\n");
			rte_pktmbuf_free(send_m);
		}
		//else
			//rte_pktmbuf_free(send_m);
#endif
			//write(node->sock, node->data, node->size);
		free(linear_buffer);
		// addNode(linear_buffer, next_send_time, sock,  size, tp_now);
	}
	return rval;
}
int tap_ind (interface, buffer, max_len, offset, rqts)
struct _interface *interface;
struct _ll_queue_element **buffer;
int max_len;
int *offset;
scps_np_rqts *rqts;
{
#if 0
	printf ("DST %02x %02x %02x %02x %02x %02x\n",
                rqts->dst_mac_addr [0],
                rqts->dst_mac_addr [1],
                rqts->dst_mac_addr [2],
                rqts->dst_mac_addr [3],
                rqts->dst_mac_addr [4],
                rqts->dst_mac_addr [5]);

        printf ("SRC %02x %02x %02x %02x %02x %02x\n",
                rqts->src_mac_addr [0],
                rqts->src_mac_addr [1],
                rqts->src_mac_addr [2],
                rqts->src_mac_addr [3],
                rqts->src_mac_addr [4],
                rqts->src_mac_addr [5]);
#endif
  	int interface_side = -1;
  	int fd = -1;
	unsigned short CRC_num=0;
	ip_template *ip = NULL;

  	memcpy (&(rqts->dst_mac_addr [0]),
          ((*buffer)->data + (*buffer)->offset + 0),
          MAC_ADDR_SIZE);

  	memcpy (&(rqts->src_mac_addr [0]),
          ((*buffer)->data + (*buffer)->offset + MAC_ADDR_SIZE),
          MAC_ADDR_SIZE);

	if( (0x81==(*((*buffer)->data+(*buffer)->offset+START_OF_FRAME_TYPE))) && (0x00==(*((*buffer)->data+(*buffer)->offset+START_OF_FRAME_TYPE+1))) )
	{
		rqts->TPID = ntohs( *((unsigned short*)((*buffer)->data+(*buffer)->offset+START_OF_FRAME_TYPE)) );
		rqts->TCI  = ntohs( *((unsigned short*)((*buffer)->data+(*buffer)->offset+START_OF_FRAME_TYPE+2)) );
  		rqts->frame_type = (
          (((int)*((*buffer)->data + (*buffer)->offset + START_OF_FRAME_TYPE + 4)) * 256) +
          (((int)*((*buffer)->data + (*buffer)->offset + START_OF_FRAME_TYPE + 5))));
	}
	else
	{
		rqts->TPID = 0;   //This is very important!
		rqts->TCI  = 0;
  		rqts->frame_type = (
          (((int)*((*buffer)->data + (*buffer)->offset + START_OF_FRAME_TYPE)) * 256) +
          (((int)*((*buffer)->data + (*buffer)->offset + START_OF_FRAME_TYPE + 1))));
	}
	memset((char*)(rqts->mpls), 0, MAX_MPLS_DEPTH*4);   //This is very important!
	rqts->mpls_depth = 0;	

  	if ((*buffer)->divert_port_number == interface->div_a_port) 
	{
		rqts->recv_tap_if = interface->tap_a_fd;
		rqts->peer_tap_if = interface->tap_b_fd;
		rqts->divert_port_number = gw_ifs.aif_divport;
		interface_side = AIF;
		fd = interface->tap_b_fd;
  	} 
	else if ((*buffer)->divert_port_number == interface->div_b_port) 
	{
		rqts->recv_tap_if = interface->tap_b_fd;
		rqts->peer_tap_if = interface->tap_a_fd;
		rqts->divert_port_number = gw_ifs.bif_divport;
		interface_side = BIF;
		fd = interface->tap_a_fd;
  	}

	rqts->CRC_cksum = 1;

  	switch (in_data->frame_type) 
	{
		case 0x0806:
		{
			int fd;
			int rc;
		
			if ((*buffer)->divert_port_number == interface->div_a_port) 
			{
				fd = interface->tap_b_fd;
			} 
			else if ((*buffer)->divert_port_number == interface->div_b_port) 
			{
				fd = interface->tap_a_fd;
			} 
			else 
			{
				printf ("%s %d ERROR in TAP_ind bad divert_port_number\n",__FILE__, __LINE__);
		      free_ll_queue_element (rqts->interface, *buffer);
				return (0);
			}

			rc = ll_tap_qk_send (fd, ((*buffer)->data +(*buffer)->offset), (*buffer)->size); 
      			free_ll_queue_element (rqts->interface, *buffer);
			return (0);
		}
		break;

      case 0x86dd:		/* Need to check for the IPv6 equivalent of ARP for IPv6 (Router Solicitation and Router Advertisement */
		{
			int fd;
			int rc;
		
			if ((*buffer)->divert_port_number == interface->div_a_port) 
			{
				fd = interface->tap_b_fd;
			} 
			else if ((*buffer)->divert_port_number == interface->div_b_port) 
			{
				fd = interface->tap_a_fd;
			} 
			else 
			{
				printf ("%s %d ERROR2 in TAP_ind bad divert_port_number\n",__FILE__, __LINE__);
		      free_ll_queue_element (rqts->interface, *buffer);
				return (0);
			}

			rc = ll_tap_qk_send (fd, ((*buffer)->data +(*buffer)->offset), (*buffer)->size); 
      			free_ll_queue_element (rqts->interface, *buffer);
			return (0);

#if 0
      	(*buffer)->offset +=SIZE_OF_ETHER_PART;
         (*buffer)->size -=SIZE_OF_ETHER_PART;
         *offset +=SIZE_OF_ETHER_PART;

         if ((int)*((*buffer)->data + (*buffer)->offset + 6) == 0x3a) /* ICMP */
			{ 
         	if ( ((unsigned char )*((*buffer)->data + (*buffer)->offset + 40) == 135) ||  /* 135 */
                             ((unsigned char )*((*buffer)->data + (*buffer)->offset + 40) == 136) ) 	/* 136 */
				{ 
            	int fd;
               int rc;

               if ((*buffer)->divert_port_number == interface->div_a_port) 
					{
               	fd = interface->tap_b_fd;
               				}	 
					else if ((*buffer)->divert_port_number == interface->div_b_port) 
					{
               	fd = interface->tap_a_fd;
  				       	} 
					else 
					{
           	  		printf ("%s %d ERROR in TAP_ind bad divert_port_number\n", __FILE__, __LINE__);
           	 		return (0);
                			}

               (*buffer)->offset -=SIZE_OF_ETHER_PART;
               (*buffer)->size +=SIZE_OF_ETHER_PART;
               *offset -=SIZE_OF_ETHER_PART;

               rc = ll_tap_qk_send (fd, ((*buffer)->data +(*buffer)->offset), (*buffer)->size);
               free_ll_queue_element (rqts->interface, *buffer);
               return (0);
                		}
                	}

         return ((*buffer)->size);
#endif
		}
      break;

		case 0x0800:
		{
			if(0 != rqts->TPID)   //vlan
			{
				(*buffer)->offset += SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG;
				(*buffer)->size -= (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG);
				*offset += SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG;
			}
			else
			{
				(*buffer)->offset +=SIZE_OF_ETHER_PART;
				(*buffer)->size -=SIZE_OF_ETHER_PART;
				*offset +=SIZE_OF_ETHER_PART;
				/********************added by YGX************************/
#if 0
				if((*buffer)->divert_port_number == interface->div_b_port)
				{
					ip = (in_Header *) ((*buffer)->data + (*buffer)->offset);
					if ( (inv4_GetProtocol (ip) == SCPSTP) && (*(((*buffer)->data + 9)) == 0x88) )
					{
						CRC_num = CRC_16 (((*buffer)->data + (*buffer)->offset), (*buffer)->size);
						//fprintf(fp_cksum, "size = %d\n", (*buffer)->size);
						//fflush(fp_cksum);
						//fprintf(fp_cksum, "(*buffer)->data = %x, CRC_num = %x!\n", ntohs(*((unsigned short*)((*buffer)->data + 10))), CRC_num);
						//fflush(fp_cksum);
						if( ntohs(*((unsigned short*)((*buffer)->data + 10))) != CRC_num)
						{
							rqts->CRC_cksum = 0;
							//fprintf(fp_cksum, "(*buffer)->data = 0x%x, CRC_num = 0x%x!\n", ntohs(*((unsigned short*)((*buffer)->data + 10))), CRC_num);
							//fflush(fp_cksum);
						}
						else
						{
							rqts->CRC_cksum = 1;
						}	
					}
				}
				else
					rqts->CRC_cksum = 1;
#endif

				/******************************************************************************************/
			}
			return ((*buffer)->size);
		}
		break;
		
		case 0x8847:       //MPLS unicast
		case 0x8848:       //MPLS multicast
		{
			unsigned int local_cnt=0;			

			if(0 != rqts->TPID)
			{
				rqts->mpls[local_cnt] = *((unsigned int*)((*buffer)->data+(*buffer)->offset+SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG));

				while( (!(rqts->mpls[local_cnt] & 0x00000100)) && (local_cnt<MAX_MPLS_DEPTH-1) )
				{
					local_cnt++;
					rqts->mpls[local_cnt] = *((unsigned int*)((*buffer)->data+(*buffer)->offset+SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG+local_cnt*SIZE_OF_MPLS_TAG));
				}

				rqts->mpls_depth = local_cnt + 1;

				(*buffer)->offset += (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG + rqts->mpls_depth*SIZE_OF_MPLS_TAG);
				(*buffer)->size -= (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG + rqts->mpls_depth*SIZE_OF_MPLS_TAG);
				*offset += (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG + rqts->mpls_depth*SIZE_OF_MPLS_TAG);
			}
			else
			{
				rqts->mpls[local_cnt] = *((unsigned int*)((*buffer)->data+(*buffer)->offset+SIZE_OF_ETHER_PART));

				while( (!(ntohl(rqts->mpls[local_cnt]) & 0x00000100)) && (local_cnt<MAX_MPLS_DEPTH-1) )
				{
					local_cnt++;
					rqts->mpls[local_cnt] = *((unsigned int*)((*buffer)->data+(*buffer)->offset+SIZE_OF_ETHER_PART+local_cnt*SIZE_OF_MPLS_TAG));
				}

				rqts->mpls_depth = local_cnt + 1;

				(*buffer)->offset += (SIZE_OF_ETHER_PART + rqts->mpls_depth*SIZE_OF_MPLS_TAG);
				(*buffer)->size -= (SIZE_OF_ETHER_PART + rqts->mpls_depth*SIZE_OF_MPLS_TAG);
				*offset += (SIZE_OF_ETHER_PART + rqts->mpls_depth*SIZE_OF_MPLS_TAG);
#if 0
				int temp = 0;
				printf("\nrqts->recv_tap_if: %d\n", rqts->recv_tap_if);
				printf("tap_ind: Get MPLS Packet!!!!!!!!!! mpls_depth=%d!!!, frame_type=0x%x\n", rqts->mpls_depth,rqts->frame_type);
				for(temp=0; temp<(*buffer)->size; temp++)
				{
					printf ("%2x ", (*buffer)->data[temp]);
 					if ((temp + 1) % 16 == 0) 
					{
						printf ("\n");
					}
				}
				printf ("\n");
#endif
			}

			return ((*buffer)->size);
		}
		break;

		default:
		{
/*
			if (gw_ifs.c_other_proto_non_ip == 1) 
			{
				other_proto_non_ip_Handler (interface_side, buffer, rqts, fd);
				return (0);
			}
      	free_ll_queue_element (rqts->interface, *buffer);
			return (0);
*/
			int fd;
			int rc;
		
			if ((*buffer)->divert_port_number == interface->div_a_port) 
			{
				fd = interface->tap_b_fd;
			} 
			else if ((*buffer)->divert_port_number == interface->div_b_port) 
			{
				fd = interface->tap_a_fd;
			} 
			else 
			{
				printf ("%s %d ERROR3 in TAP_ind bad divert_port_number\n",__FILE__, __LINE__);
		      free_ll_queue_element (rqts->interface, *buffer);
				return (0);
			}

			rc = ll_tap_qk_send (fd, ((*buffer)->data +(*buffer)->offset), (*buffer)->size); 
      	free_ll_queue_element (rqts->interface, *buffer);
			return (0);
		}
		break;
  	}

  	return (0);
}

#endif /* TAP_INTERFACE */

Node *head = NULL;
Node *tail = NULL;
uint32_t NodeCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Node* createNode(uint8_t *data, uint32_t time, uint32_t sock, uint32_t size, uint32_t timesample) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->sock = sock;
    newNode->time = time;
    newNode->size = size;
    newNode->timesample = timesample;
    newNode->next = NULL;
    return newNode;
}

void addNode(uint8_t *data, uint32_t time, uint32_t sock, uint32_t size, uint32_t timesample) {
    //pthread_mutex_lock(&mutex);
    Node *newNode = createNode(data, time, sock, size, timesample);
    if (head == NULL) {
        head = (Node*)malloc(sizeof(Node)); // 创建哨兵节点
        head->next = newNode;
        tail = newNode;
    } else if (tail == NULL) {
        head->next = newNode;
        tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
    NodeCount++;
    //pthread_mutex_unlock(&mutex);
}

void freeNode(Node *node, uint32_t time) {
    if (node->data != NULL) {
       

#if 1
		struct rte_mbuf *send_m = rte_pktmbuf_alloc(l2fwd_pktmbuf_pool);
		if (send_m == NULL)
			printf("Alloc pktmbuf failed!!!\n");

		memcpy(rte_pktmbuf_mtod(send_m, void *), node->data, node->size);
		send_m->nb_segs = 1;
		send_m->next = NULL;
		send_m->pkt_len = (uint16_t)node->size;
		send_m->data_len = (uint16_t)node->size;


		//printf("tap_send: dstport:%d\n", sock);
		int rval = rte_eth_tx_burst(node->sock, 0, &send_m, 1);	
		if (unlikely(rval < 1)) 
		{	
			printf("dpdk drop one packet!!\n");
			rte_pktmbuf_free(send_m);
		}
		//else
			//rte_pktmbuf_free(send_m);
#endif
			//write(node->sock, node->data, node->size);
		free(node->data);
    }
    free(node);
    NodeCount--;
}



void checkAndFreeNodes() {
    //pthread_mutex_lock(&mutex);
    uint32_t time = get_time_us();
    Node *cur = head->next;
    Node *prev = head;

    while (cur != NULL) {
	//for(int i = 0; i < 200;i++);
	 
	//printf("%u\n",time);
        if (time >= cur->time) {
            prev->next = cur->next;
            if (cur == tail) {
                tail = prev;
            }
            Node *temp = cur;
            cur = cur->next;
            freeNode(temp, time);
        } else {
            prev = cur;
            cur = cur->next;

        }
    }

    if (head->next == NULL) {
        tail = NULL;
    }

    //pthread_mutex_unlock(&mutex);
}

int tap_thread_func(void *p)
{
	// addNode(NULL, 0xffffffff, 0, 0, 0);
	// for(;;)
	// {
	// 	checkAndFreeNodes();
	// }
	int i = 0;
	for(;;)
	{
		void *p = my_malloc(sizeof(Data));
		while (rte_ring_dequeue(ring, &p) < 0) {
            //printf("Failed to dequeue element\n");
		}
		Data *d = (Data *)(p);
		while(1)
		{
			if(d->time <= get_time_us())
			{
	#if 1
				struct rte_mbuf *send_m = rte_pktmbuf_alloc(l2fwd_pktmbuf_pool);
				if (send_m == NULL)
					printf("Alloc pktmbuf failed!!!\n");
	
				memcpy(rte_pktmbuf_mtod(send_m, void *), d->data, d->size);
				send_m->nb_segs = 1;
				send_m->next = NULL;
				send_m->pkt_len = (uint16_t)d->size;
				send_m->data_len = (uint16_t)d->size;
	
	
				//printf("tap_send: dstport:%d\n", sock);
				int sent_pkts = 0;
				// while (sent_pkts == 0) {
				// 	// 调用 rte_eth_tx_burst 发送单个数据包
				// 	sent_pkts = rte_eth_tx_burst(d->sock, 0, &send_m, 1);	
			
				// 	if (sent_pkts == 0) {
				// 		// 如果发送失败，可能是队列已满或硬件问题
				// 		// 在这里添加错误处理逻辑，例如暂停一段时间后重试
				// 		printf("dpdk drop one packet!!!\n");//rte_delay_us(10); // 暂停100微秒
				// 	}
				// }

				int rval = rte_eth_tx_burst(d->sock, 0, &send_m, 1);	
				if (unlikely(rval < 1)) 
				{	
					printf("dpdk drop one packet!!!\n");
					rte_pktmbuf_free(send_m);
				}
				rte_pktmbuf_free(send_m);
				my_free(d->data);
				my_free(p);
				for(i = 0; i < 200; i++);
				break;
	#endif
					//write(node->sock, node->data, node->size);
				
			}
		}
		
	}

	return 0;
}
