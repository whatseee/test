#include "../include/scps.h"
#include "scpstp.h"
#include "scpsudp.h"

extern void my_free (void *ptr);
extern int printf (const char *format, /* args */ ...);

int scps_np_trequest (tp_Socket * s, scps_ts * ts, route * nproute, uint32_t
		      length, struct mbuff *m, u_char th_off);

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: udp_utility.c,v $ -- $Revision: 1.16 $\n";
#endif

extern route *def_route;
extern uint32_t tp_now;
extern udp_Socket *udp_allsocs;
extern int udp_write_count;
extern int udp_send_count;
extern unsigned short udp_id;
extern short global_conn_ID;

extern int nl_default;

int
udp_Common (udp_Socket * s)
{
  int temp;
  struct threads *udpthread;
  struct mbuff *mbuffer;

  /* We're clean, skip the rest */
  if (s->Initialized)
    return (0);

  /* Get a pseudo-file descriptor for our socket */
  for (temp = 0; temp < MAX_SCPS_SOCKET; temp++)
    {
      if (scheduler.sockets[temp].ptr == NULL)
	{
	  scheduler.sockets[temp].ptr = (caddr_t) s;
	  s->sockid = temp;
	  scheduler.num_of_socket++;
	  //printf("udp_Common: num_of_socket==%d !!!\n", scheduler.num_of_socket);
	  break;
	}
    }

  if (temp == MAX_SCPS_SOCKET)
    {
	SET_ERR (SCPS_ENOBUFS);
	return (-1);
    }

  s->thread = scheduler.current;	/* process id of this thread */
  tp_now = clock_ValueRough ();
  s->total_data = 0;
  s->th_off = temp = MBLEN - UDP_HDR_LEN;
  s->sockFlags |= SOCK_BL;	/* Sockets block by default */

/* Set some default network layer parameters */
  s->np_rqts.tpid = SCPSUDP;
  s->np_rqts.ipv4_dst_addr = 0;
  s->np_rqts.ipv4_src_addr = 0;
  s->np_rqts.timestamp.format = 0;
  s->np_rqts.timestamp.ts_val[0] = s->np_rqts.timestamp.ts_val[1] = 0;
  s->np_rqts.bqos.precedence = 0;
  s->np_rqts.bqos.routing = 0;
  s->np_rqts.bqos.pro_specific = 0;
  s->np_rqts.eqos.ip_precedence = 0;
  s->np_rqts.eqos.ip_tos = 0;
  s->np_rqts.cksum = 1;
  s->np_rqts.int_del = 0;

  if (!s->np_rqts.nl_protocol) {
      s->np_rqts.nl_protocol =  nl_default;
  }

#ifdef SCPSSP
/* Set some defaulta security layer parameters */
  s->sp_rqts.np_rqts.tpid = SP;
  s->sp_rqts.np_rqts.ipv4_dst_addr = 0;
  s->sp_rqts.np_rqts.ipv4_src_addr = 0;
  s->sp_rqts.np_rqts.timestamp.format = 0;
  s->sp_rqts.np_rqts.timestamp.ts_val[0] =
    s->sp_rqts.np_rqts.timestamp.ts_val[1] = 0;
  s->sp_rqts.np_rqts.bqos.precedence = s->np_rqts.bqos.precedence;
  s->sp_rqts.np_rqts.bqos.routing = 0;
  s->sp_rqts.np_rqts.bqos.pro_specific = 0;
  s->sp_rqts.np_rqts.eqos.ip_precedence = 0;
  s->sp_rqts.np_rqts.eqos.ip_tos = 0;
  s->sp_rqts.np_rqts.cksum = 1;
  s->sp_rqts.np_rqts.int_del = 0;
  s->sp_rqts.tpid = SCPSUDP;
  s->sp_rqts.sprqts = 0x00;
  s->np_rqts.tpid = SP;

  if (!s->np_rqts.nl_protocol) {
      s->sp_rqts.np_rqts.nl_protocol =  nl_default;
  }

  s->sp_size = sp_hdr_size (s->sp_rqts);
  temp = s->sp_size + (s->sp_size % sizeof (uint32_t));
  s->sh_off = s->th_off - temp;
  temp = s->sh_off;
#endif /* SCPSSP */

  s->np_size = np_hdr_size (s->np_rqts);
  s->nh_off = temp - s->np_size - (s->np_size % sizeof (uint32_t));

  s->rt_route = def_route;	/* temporary */

  s->ph.nl_head.ipv4.src = local_addr;
  s->ph.nl_head.ipv4.mbz = 0;
  s->ph.nl_head.ipv4.protocol = SCPSUDP;
  s->ph.nl_head.ipv4.upper_seq_num = 0;	/* to be safe */
  s->ph.nl_head.ipv4.dst = 0;

  s->receive_buff = buff_init (MAX_MBUFFS, s);
  s->send_buff = buff_init (1, s);	/* just need a single mbuff for sending */

  s->app_sbuff = chain_init (MAX_UDP_PAYLOAD);	/* big enough for 1 datagram */

  s->app_rbuff = chain_init (BUFFER_SIZE);
  s->buff_full = FALSE;

  if (!(mbuffer = alloc_mbuff (MT_HEADER)))
    {
      SET_ERR (SCPS_ENOBUFS);
      return (-1);
    }

  /* initialize the send buffer with a single mbuff */
  if (!(enq_mbuff (mbuffer, s->send_buff)))
    printf ("MBUFFER ENQUEUEING ERROR\n");

#ifndef GATEWAY_SINGLE_THREAD
  udpthread = get_thread (tp);	/* mark tp thread as runnable (we need it) */
  if (udpthread->status > Ready)
    udpthread->status = Ready;
#endif /* GATEWAY_SINGLE_THREAD */

  gettimeofday (&(s->start_time), NULL);

  s->Initialized = SCPSUDP;
  s->select_timer = 0;
  s->next = udp_allsocs;
  if (s->next)
    s->next->prev = s;
  s->prev = NULL;
  udp_allsocs = s;

  return (s->sockid);
}

/*
 * Unthread a UDP socket from the socket list, if it's there.
 */
void udp_Unthread (ds)
udp_Socket *ds;
{
	udp_Socket *s, **sp;

	ds->Initialized = 0;
	sp = &udp_allsocs;

	scheduler.sockets[ds->sockid].ptr = NULL;
	scheduler.num_of_socket--;
	//printf("udp_Unthread: num_of_socket==%d !!!\n", scheduler.num_of_socket);

	for (;;)
    {
		s = *sp;
		if (s == ds)
		{
			kill_bchain (s->send_buff->start);
			kill_bchain (s->receive_buff->start);

			s->receive_buff->start = s->receive_buff->last = NULL;

			*sp = s->next;

			my_free (s);

			break;
		}

		if (s == NIL)
			break;

		sp = &(s->next);
    }

	Validate_Thread ();
}

void
udp_DumpHeader (ip, upp, mesg)
     in_Header *ip;
     udp_Header *upp;
     char *mesg;
{
  register udp_Header *up;

  up = upp;

  printf ("UDP: %s packet: S: %x; D: %x; Len=%d ",
	  mesg, ntohs (up->srcPort), ntohs (up->dstPort),
	  ntohs ((u_short) up->len));
  printf ("C=%x\n", ntohs (up->checksum));
}
