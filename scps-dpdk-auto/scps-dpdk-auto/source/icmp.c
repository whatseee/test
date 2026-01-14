#ifdef IP_ICMP

#include "../include/scps.h"
#include "scps_ip.h"
#include "scpstp.h"
#include "scpsudp.h"
#include "ll.h"

#ifdef __FreeBSD__
#define __ISBSDISH__
#endif /* __FreeBSD__ */
#ifdef NETBSD
#define __ISBSDISH__
#endif /* NETBSD */

#ifdef __OpenBSD__
#define __ISBSDISH__
#endif /* __OpenBSD__ */

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: icmp.c,v $ -- $Revision: 1.10 $\n";
#endif

extern void *memset (void *s, int c, size_t n);
extern struct msghdr out_msg;
extern struct _ll_queue_element *in_data;

#ifdef GATEWAY
#include "rs_config.h"
extern GW_ifs gw_ifs;
#endif /* GATEWAY */

extern struct _interface *sock_interface;
extern struct _interface *divert_interface;
extern tp_Socket *tp_allsocs;   /* Pointer to first TP socket */

extern int scps_udp_port;
void cause_reset (unsigned char *pkt);

void
icmp_Handler (rqts_in, ip_pkt_len, hdr, offset)
scps_np_rqts *rqts_in;
int ip_pkt_len;
ip_template *hdr;  
int offset;

{

  icmp_Header *i_hdr;
  int keep_it = -1;
#ifdef DIVERT
  int rc;
  struct sockaddr_in sin;
#endif /* DIVERT */

  i_hdr = (icmp_Header *) ((void *) in_data->data + offset);

  switch (i_hdr -> icmp_type) {

    case ICMP_ECHOREPLY:
    case ICMP_SOURCEQUENCH:
    case ICMP_REDIRECT:
    case ICMP_ECHO:
    case ICMP_ROUTERADVERT:
    case ICMP_ROUTERSOLICIT:
    case ICMP_TIMXCEED:
    case ICMP_PARAMPROB:
    case ICMP_TSTAMP:
    case ICMP_TSTAMPREPLY:
    case ICMP_IREQ:
    case ICMP_IREQREPLY:
    case ICMP_MASKREQ:
    case ICMP_MASKREPLY:
	keep_it = 0;
	break;

    case ICMP_UNREACH:
        keep_it = icmp_unreachable (i_hdr);
	break;
    default:
	printf ("ICMP unknown option\n");
	break;

  }
  
  if (keep_it == 0) {

#ifdef DIVERT
        memset (&sin, 0x00, sizeof (sin));

#if defined (FREEBSD) || defined(__OpenBSD__)
	sin.sin_len = sizeof (sin);
#endif /* FREEBSD */
	sin.sin_family = AF_INET;
	sin.sin_port = (short) (htons (0000));
        sin.sin_addr.s_addr = ntohl (0x7f000001);
	
	
        rc = sendto  (divert_interface->div_socket, (unsigned char *) hdr,
                      ip_pkt_len, 0x0, (struct sockaddr *) &sin, sizeof (sin));

#endif /* DIVERT */

#ifdef  TAP_INTERFACE
#ifdef PRINT_PKT
{
        int i;
      
        for (i = 0; i < in_data->frame_size; i++) {
                printf ("%2x ", in_data->data [i]);
                if ((i + 1) % 16 == 0) {
                        printf ("\n");
                }
        }
        printf ("\n");
}
#endif /* PRINT_PKT */
        other_proto_Handler (in_data->interface , &in_data, MAX_MTU, &offset, rqts_in, ICMP);

//        rc = ll_tap_qk_send (fd, ((*buffer)->data +(*buffer)->offset), (*buffer)->size);
#endif /*  TAP_INTERFACE */

  }

  if (keep_it == 1) {

#ifdef PRINT_PKT
{
        int i;
      
        for (i = 0; i < in_data->frame_size; i++) {
                printf ("%2x ", in_data->data [i]);
                if ((i + 1) % 16 == 0) {
                        printf ("\n");
                }
        }
        printf ("\n");
}
#endif /* PRINT_PKT */
#ifdef GATEWAY
	cause_reset ((void *) in_data->data + offset + 8);
#endif /* GATEWAY */
  }

  if (keep_it == -1) {
	printf ("unknown code\n");

  }

  return;
}

#ifdef GATEWAY
void
cause_reset (unsigned char *pkt)

{
  tp_Socket *s;
  ip_template *ip;
  tp_Header *tp;  
  scps_np_rqts arqts;
  scps_np_rqts *rqts = &arqts;


  ip = (in_Header *) ((void *) pkt);

  if (inv4_GetProtocol (ip) == SCPSTP) {
     tp = (tp_Header *) (((void*) ip + 20));
  } else  {
     return;
  }


  for (s = tp_allsocs; s; s = s->next) {
    if (ntohs (tp->dstPort) == ntohs (s->hisport) &&
        ntohs (tp->srcPort) == ntohs (s->myport) &&
        ntohl (ip->nl_head.ipv4.destination) == ntohl (s->his_ipv4_addr) &&
        ntohl (ip->nl_head.ipv4.source) == ntohl (s->my_ipv4_addr)) {
        break;
      }
  }

  if (s) {
          rqts->tpid = inv4_GetProtocol (ip);
          rqts->ipv4_src_addr = ntohl (ip->nl_head.ipv4.destination);
          rqts->ipv4_dst_addr = ntohl (ip->nl_head.ipv4.source);
          /* rqts->timestamp = ; */
          /* rqts->bqos = ; */
          /* rqts->eqos = ; */
          rqts->cksum = 0;
          /* rqts->int_del = ; */
          rqts->nl_protocol = NL_PROTOCOL_IPV4;
          rqts->DSCP = (unsigned char ) ((ntohs (ip->nl_head.ipv4.vht)) & 0x00ff);
	  tp_Abort (s->sockid);
#ifdef GATEWAY
	  if (s->peer_socket) {
	      tp_Abort (s->peer_socket->sockid);
	  }
#endif /* GATEWAY */
   }
}
#endif /* GATEWAY */



int
scps_icmp_output ()

{

  return (0);
}


int
icmp_unreachable (i_hdr)
icmp_Header *i_hdr;

{
  ip_template *ip;
  int rc = 0;
  udp_Header *uh;

  ip = ( (ip_template *) ((void *) (i_hdr) + 8));

  if (inv4_GetProtocol (ip) == SCPSTP) {
	rc = 1;
  }

  if (inv4_GetProtocol (ip) == SCPSUDP) {
    uh = (udp_Header *) ((void*) ip + 20);

    if ((ntohs (uh->srcPort) == scps_udp_port)  ||
        (ntohs (uh->dstPort) == scps_udp_port)) {
      rc = 1;

    printf ("SYSTEM ERROR: Peer gateway IP (%x) may be down\n",
             (unsigned int) (ntohl (ip->nl_head.ipv4.source)));
    }
  }

  return (rc);
}

#endif /* IP_ICMP */
