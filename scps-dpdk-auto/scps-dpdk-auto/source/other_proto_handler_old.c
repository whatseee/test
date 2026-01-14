#ifdef GATEWAY

#include "scps.h"
#include "scpstp.h"
#include "scpserrno.h"
#include "tp_debug.h"
#include "gateway.h"
#include <stdio.h>
#include <math.h>


#include "other_proto_handler.h"

#ifdef SCPSSP
#include "scps_sp.h"
#endif /* SCPSSP */

#ifdef GATEWAY_ROUTER
#include "rt_alloc.h"
#endif /* GATEWAY_ROUTER */

#ifdef TAP_INTERFACE
#include "tap.h"
#endif /* TAP_INTERFACE */

#include "scps_ip.h"
#include "scps_np.h"
#include "scps_defines.h"

int scps_np_get_template (scps_np_rqts * rqts,
			  scps_np_template * templ);
 
#ifdef GATEWAY
#include "rs_config.h"
extern GW_ifs gw_ifs;
int init_port_number_offset;
extern route *def_route;
extern route *other_route;
#endif /* GATEWAY */

#ifdef Sparc
#ifndef SOLARIS
extern int gettimeofday (struct timeval *tp, struct timezone *tzp);
#endif /* SOLARIS */
#endif /* Sparc */

extern struct _interface *sock_interface;
extern struct _interface *divert_interface;

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: other_proto_handler.c,v $ -- $Revision: 1.10 $\n";
#endif

extern tp_Socket *tp_allsocs;	/* Pointer to first TP socket */
extern uint32_t tp_now;
extern struct timeval end_time;
extern float elapsed_time;
extern int delayed_requested;
int delayed_sent;
extern short tp_id;
extern int cluster_check;

struct _ll_queue_element *in_data;      /* packet buffer */
extern GW_ifs gw_ifs;

int abs (int i);		/* test function prototype */

other_proto_q_t other_proto [PROTO_MAX] [2];
other_proto_q_t other_proto_non_ip [2];
other_proto_q_t other_proto_ipv6 [2];

/*
 * Handler for incoming TP packets.
 */
void other_proto_init ()
{
    int i, j;

	for (i = 0; i < PROTO_MAX; i++) 
	{
		for (j = 0; j < 2; j++) 
		{
			other_proto [i] [j]. q = q_create ();
			other_proto [i] [j]. def_rt = NULL;
			other_proto [i] [j]. rt = NULL;
		}
	}

    for (j = 0; j < 2; j++) 
	{
        other_proto_non_ip [j]. q = q_create ();
        other_proto_non_ip [j]. def_rt = NULL;
        other_proto_non_ip [j]. rt = NULL;
    }

    for (j = 0; j < 2; j++) 
	{
        other_proto_ipv6 [j]. q = q_create ();
        other_proto_ipv6 [j]. def_rt = NULL;
        other_proto_ipv6 [j]. rt = NULL;
    }
}

void other_proto_Handler (interface, buffer, max_len, offset, rqts, proto)
struct _interface *interface;
struct _ll_queue_element **buffer;
int max_len;
int *offset;
scps_np_rqts *rqts;
unsigned char proto;
{
	other_proto_pkt_t *pkt;
	int interface_side = -1;
	int fd;

#ifdef TAP_INTERFACE
	if (rqts->divert_port_number == gw_ifs.aif_divport) 
	{
		interface_side = BIF;
		fd = interface->tap_b_fd;
        other_proto [proto] [interface_side].rt = other_route;
        other_proto [proto] [interface_side].def_rt = other_route;
	} 
	else 
	{
		interface_side = AIF;
		fd = interface->tap_a_fd;
        other_proto [proto] [interface_side].rt = def_route;
        other_proto [proto] [interface_side].def_rt = def_route;
	}
#else
	if ((*buffer)->divert_port_number == gw_ifs.aif_divport) 
	{
		interface_side = BIF;
		fd = interface->tun_c_fd;
		other_proto [proto] [interface_side].rt = other_route;
        other_proto [proto] [interface_side].def_rt = other_route;
	}
	else 
	{
		interface_side = AIF;
		fd = interface->tun_c_fd;
		other_proto [proto] [interface_side].rt = def_route;
        other_proto [proto] [interface_side].def_rt = def_route;
	}
#endif
/*
  if (q_empty (other_proto [proto] [interface_side].q)) {
	other_proto [proto] [interface_side] .rt =
	       other_proto_get_route ();
  }
*/

	if (other_proto [proto] [interface_side].q->q_len <= gw_ifs.c_other_proto_qlen) 
	{
		pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));
		pkt->fd = fd;

#ifdef TAP_INTERFACE
		memcpy (&(pkt->src_mac_addr [0]), &(rqts->src_mac_addr [0]),6);
		memcpy (&(pkt->dst_mac_addr [0]), &(rqts->dst_mac_addr [0]),6);

		pkt->frame_type = rqts->frame_type;

		memcpy (&(pkt->data [0]), (*buffer)->data, MAX_LL_DATA);

		pkt->length = (*buffer)->size;
		pkt->offset = (*buffer)->offset;
#else
		memcpy (&(pkt->data [0]), (*buffer)->data+(*buffer)->offset, (*buffer)->size);
		pkt->length = (*buffer)->size;
#endif
		q_addt (other_proto [proto] [interface_side].q, (char *) pkt); 
	}

#ifdef TAP_INTERFACE
	free_ll_queue_element (rqts->interface, *buffer);
#else
	free_ll_queue_element (interface, *buffer);
#endif
}

void other_proto_emit ()
{
    int i, j;
  	other_proto_pkt_t *pkt;
    int rc;
	int len;

	for (i = 0; i < PROTO_MAX; i++) 
	{
		for (j = 0; j < 2; j++) 
		{
			if (q_empty (other_proto [i] [j].q)) 
			{

			} 
			else 
			{
				if (1 /*( ((other_proto_pkt_t *)other_proto [i] [j].q->q_head->qe_data)->length < other_proto [i] [j].rt->current_credit) 
#ifdef FLOW_CONTROL_THRESH
				 && ((other_proto [i] [j].rt->cong_control != FLOW_CONTROL_CONGESTION_CONTROL) ||
				    ((other_proto [i] [j].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) &&
			             (((other_proto_pkt_t *)
                                     other_proto [i] [j].q->q_head->qe_data)->length <
                                       other_proto [i] [j].rt->flow_control)))
#endif
					*/) 
				{	
					pkt = (other_proto_pkt_t *) q_deq (other_proto [i] [j].q);

#ifdef TAP_INTERFACE			
					if( (0x81==pkt->data[START_OF_FRAME_TYPE]) && (0x00==pkt->data[START_OF_FRAME_TYPE+1]) )
					{
						
       				pkt->offset -= (SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG);
       	   		pkt->length += SIZE_OF_ETHER_PART+SIZE_OF_8021Q_TAG;
						len = pkt->length;
					}
					else
					{	
       				pkt->offset -=SIZE_OF_ETHER_PART;
       	   		pkt->length +=SIZE_OF_ETHER_PART;
						len = pkt->length;
					}
					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
#else
					len = pkt->length;
					rc = write(pkt->fd, pkt->data, pkt->length);
#endif
/*
					if (rc > 0) 
					{
       					other_proto [i] [j].rt->current_credit -= len;
#ifdef FLOW_CONTROL_THRESH
				    	if (other_proto [i] [j].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) 
						{
       						other_proto [i] [j].rt->flow_control -= len;
						}
#endif
					}
*/
					my_free (pkt);
				} 
				else if (gw_ifs.c_other_proto_xrate_drop == 1) 
				{
					pkt = (other_proto_pkt_t *) q_deq (other_proto [i] [j].q);
					my_free (pkt);
				} 
				else 
				{
				}
			}
		}
	}
}

#ifdef TAP_INTERFACE

void other_proto_non_ip_Handler (interface_side, buffer, rqts, fd)
int interface_side;
struct _ll_queue_element **buffer;
scps_np_rqts *rqts;
int fd;
{
    other_proto_pkt_t *pkt;

	if (other_proto_non_ip [interface_side].q->q_len <= gw_ifs.c_other_proto_qlen) 
	{
		if (interface_side == BIF) 
		{
			other_proto_non_ip [interface_side].rt = other_route;
			other_proto_non_ip [interface_side].def_rt = other_route;
		} 
		else 
		{
			other_proto_non_ip [interface_side].rt = def_route;
			other_proto_non_ip [interface_side].def_rt = def_route;
		}

		pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));
		pkt->fd = fd;
		memcpy (&(pkt->data [0]), (*buffer)->data, (*buffer)->size);
		pkt->length = (*buffer)->size;
		pkt->offset = (*buffer)->offset;
		q_addt (other_proto_non_ip [interface_side].q, (char *) pkt);
	}
        
	free_ll_queue_element (rqts->interface, *buffer);
}

void other_proto_non_ip_emit ()
{
	int j;
  	other_proto_pkt_t *pkt;
    int rc;

	for (j = 0; j < 2; j++) 
	{
		if (q_empty (other_proto_non_ip [j].q)) 
		{

		} 
		else 
		{
			if (1 /*( ((other_proto_pkt_t *)other_proto_non_ip [j].q->q_head->qe_data)->length < other_proto_non_ip [j].rt->current_credit) 
#ifdef FLOW_CONTROL_THRESH
	   		    && ((other_proto_non_ip  [j].rt->cong_control != FLOW_CONTROL_CONGESTION_CONTROL) ||
		    	       ((other_proto_non_ip [j].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) &&
			       (((other_proto_pkt_t *)other_proto_non_ip [j].q->q_head->qe_data)->length < other_proto_non_ip [j].rt->flow_control)))
#endif
				*/) 
			{	
				pkt = (other_proto_pkt_t *) q_deq (other_proto_non_ip [j].q);
	
				rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
/*
				if (rc > 0) 
				{
       				other_proto_non_ip [j].rt->current_credit -= rc;
#ifdef FLOW_CONTROL_THRESH
				    if (other_proto_non_ip [j].rt->cong_control ==  FLOW_CONTROL_CONGESTION_CONTROL) 
					{
       					other_proto_non_ip [j] .rt->flow_control -= rc;
					}
#endif
				}
*/
				my_free (pkt);
			} 
			else if (gw_ifs.c_other_proto_xrate_drop == 1) 
			{
				pkt = (other_proto_pkt_t *) q_deq (other_proto_non_ip [j].q);
				my_free (pkt);
			} 
			else 
			{	
			}
		}
	}
}

void other_proto_ipv6_Handler (interface_side, buffer, rqts, fd)
int interface_side;
struct _ll_queue_element **buffer;
scps_np_rqts *rqts;
int fd;
{
    other_proto_pkt_t *pkt;

    if (other_proto_ipv6 [interface_side].q->q_len <= gw_ifs.c_other_proto_qlen) 
	{
        if (interface_side == BIF) 
		{
            other_proto_ipv6 [interface_side].rt = other_route;
            other_proto_ipv6 [interface_side].def_rt = other_route;
        } 
		else 
		{
            other_proto_ipv6 [interface_side].rt = def_route;
            other_proto_ipv6 [interface_side].def_rt = def_route;
        }

        pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));
        pkt->fd = fd;
        memcpy (&(pkt->data [0]), (*buffer)->data, (*buffer)->size);
        pkt->length = (*buffer)->size;
        pkt->offset = (*buffer)->offset;
        q_addt (other_proto_ipv6 [interface_side].q, (char *) pkt);
	}

    free_ll_queue_element (rqts->interface, *buffer);
}

void other_proto_ipv6_emit ()
{
    int j;
    other_proto_pkt_t *pkt;
    int rc;

    for (j = 0; j < 2; j++) 
	{
        if (q_empty (other_proto_ipv6 [j].q)) 
		{
        } 
		else 
		{
            if (1/* ( ((other_proto_pkt_t *)other_proto_ipv6 [j].q->q_head->qe_data)->length < other_proto_ipv6 [j].rt->current_credit)
#ifdef FLOW_CONTROL_THRESH
	   		    && ((other_proto_non_ip  [j].rt->cong_control != FLOW_CONTROL_CONGESTION_CONTROL) ||
		    	       ((other_proto_non_ip [j].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) &&
			       (((other_proto_pkt_t *)other_proto_non_ip [j].q->q_head->qe_data)->length < other_proto_non_ip [j].rt->flow_control)))
#endif
				*/) 
			{
                pkt = (other_proto_pkt_t *) q_deq (other_proto_ipv6 [j].q);
                rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
/*
				if (rc > 0) 
				{
                    other_proto_ipv6 [j].rt->current_credit -= rc;
#ifdef FLOW_CONTROL_THRESH
				    if (other_proto_non_ip [j].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) 
					{
       					other_proto_non_ip [j] .rt->flow_control -= rc;
					}
#endif
				}
*/
                        
				my_free (pkt);                 
			} 
			else if (gw_ifs.c_other_proto_xrate_drop == 1) 
			{
                pkt = (other_proto_pkt_t *) q_deq (other_proto_ipv6 [j].q);
                my_free (pkt);
            }
			else 
			{
            }
        }
    }
}

#endif /* TAP_INTERFACE */
#endif /* GATEWAY */
