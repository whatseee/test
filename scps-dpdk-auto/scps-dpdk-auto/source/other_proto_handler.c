#ifdef GATEWAY

#include "../include/scps.h"
#include "scpstp.h"
#include "../include/scpserrno.h"
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

int scps_np_get_template (scps_np_rqts * rqts, scps_np_template * templ);
 
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

other_proto_q_t other_proto[2];
other_proto_q_t other_proto_non_ip[2];
other_proto_q_t other_proto_ipv6[2];

//other_proto_pkt_t *pkt;
/*
 * Handler for incoming TP packets.
 */
void other_proto_init()
{
    	int i, j, k;

	//pkt = (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));
	for (j = 0; j < 2; j++) 
	{
		for(k = 0; k < MAX_MCPC_DST_STA; k++)
		{
			other_proto[j].r_and_q[k].sign = 0;
			other_proto[j].r_and_q[k].q = q_create ();
			other_proto[j].r_and_q[k].rt = NULL;
		}
	}

    	for (i = 0; i < 2; i++)
	{
		for(j = 0; j < MAX_MCPC_DST_STA; j++)
		{
			other_proto_non_ip[i].r_and_q[j].sign = 0;
			other_proto_non_ip[i].r_and_q[j].q = q_create ();
			other_proto_non_ip[i].r_and_q[j].rt = NULL;
		}
   	}
    
	for (i = 0; i < 2; i++)
	{
		for(j = 0; j < MAX_MCPC_DST_STA; j++)
		{
			other_proto_ipv6[i].r_and_q[j].sign = 0;
			other_proto_ipv6[i].r_and_q[j].q = q_create ();
			other_proto_ipv6[i].r_and_q[j].rt = NULL;
		}
    	}

	/////////////////////////////////////////////////////////////

	other_proto[AIF].r_and_q[0].sign = 1;				
	other_proto[AIF].r_and_q[0].rt = def_route;
	other_proto[BIF].r_and_q[MAX_MCPC_DST_STA-1].sign = 1;				
	other_proto[BIF].r_and_q[MAX_MCPC_DST_STA-1].rt = other_route;

	other_proto_non_ip[AIF].r_and_q[0].sign = 1;
	other_proto_non_ip[AIF].r_and_q[0].rt = def_route;
	other_proto_non_ip[BIF].r_and_q[0].sign = 1;
	other_proto_non_ip[BIF].r_and_q[0].rt = other_route;

	other_proto_ipv6[AIF].r_and_q[0].sign = 1;
	other_proto_ipv6[AIF].r_and_q[0].rt = def_route;
	other_proto_ipv6[BIF].r_and_q[0].sign = 1;
	other_proto_ipv6[BIF].r_and_q[0].rt = other_route;
}

void add_route_for_other_proto(route *rt)
{
	int k=0;

	for(k = 0; k < MAX_MCPC_DST_STA - 1; k++)
	{
		if(other_proto[BIF].r_and_q[k].sign == 0)
		{
			other_proto[BIF].r_and_q[k].sign = 1;				
			other_proto[BIF].r_and_q[k].rt = rt;
			break;
		}
	}
}

void del_route_for_other_proto(int route_sock_id)
{
	int k=0;
	other_proto_pkt_t *pkt;

	for(k = 0; k < MAX_MCPC_DST_STA - 1; k++)
	{
		if(other_proto[BIF].r_and_q[k].sign == 1)
		{
			if(other_proto[BIF].r_and_q[k].rt->route_sock_id == route_sock_id)
			{
				other_proto[BIF].r_and_q[k].sign = 0;				
				other_proto[BIF].r_and_q[k].rt = NULL;

				while(pkt = (other_proto_pkt_t *)q_deq (other_proto[BIF].r_and_q[k].q))
				{
					if(pkt)
						my_free (pkt);
				}

				break;
			}
		}
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
	int k = 0;

	uint32_t dst_ipaddr = 0;
    	uint32_t dst_netmask = 0;
    	uint32_t src_ipaddr = 0;
    	uint32_t src_netmask = 0;

	ip_template *ip = NULL;

	ip = (in_Header *)((void *)((*buffer)->data) + (*buffer)->offset);

#ifdef TAP_INTERFACE
	if (rqts->divert_port_number == gw_ifs.aif_divport) 
	{
		interface_side = BIF;
		fd = interface->tap_b_fd;
	}
	else 
	{
		interface_side = AIF;
		fd = interface->tap_a_fd;
	}
#else
	if ((*buffer)->divert_port_number == gw_ifs.aif_divport) 
	{
		interface_side = BIF;
		fd = interface->tun_c_fd;
	}
	else 
	{
		interface_side = AIF;
		fd = interface->tun_c_fd;
	}
#endif

	if(interface_side == AIF)
	{
		if (other_proto[interface_side].r_and_q[0].q->q_len <= gw_ifs.c_other_proto_qlen) 
		{
			pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));

			if(!pkt)
			{
				printf("Gateway-->Function other_proto_Handler: my_malloc1 return NULL\n");
				fflush(stdout);
#ifdef TAP_INTERFACE
				free_ll_queue_element (rqts->interface, *buffer);
#else
				free_ll_queue_element (interface, *buffer);
#endif
				return;
			}

			pkt->fd = fd;

#ifdef TAP_INTERFACE
			memcpy (&(pkt->src_mac_addr [0]), &(rqts->src_mac_addr [0]),6);
			memcpy (&(pkt->dst_mac_addr [0]), &(rqts->dst_mac_addr [0]),6);

			pkt->frame_type = rqts->frame_type;
			pkt->mpls_depth = rqts->mpls_depth;  //MPLS

			memcpy (&(pkt->data [0]), (*buffer)->data, MAX_LL_DATA);

			pkt->length = (*buffer)->size;
			pkt->offset = (*buffer)->offset;
#else
			memcpy (&(pkt->data [0]), (*buffer)->data+(*buffer)->offset, (*buffer)->size);
			pkt->length = (*buffer)->size;
#endif

			q_addt (other_proto[interface_side].r_and_q[0].q, (char *) pkt); 
		}
	}

	if(interface_side == BIF)
	{
		for(k = 0; k < MAX_MCPC_DST_STA; k++)
		{
			if(other_proto[interface_side].r_and_q[k].sign == 1)
			{
				dst_ipaddr = other_proto[interface_side].r_and_q[k].rt->dst_ipaddr;
				dst_netmask = other_proto[interface_side].r_and_q[k].rt->dst_netmask;
				src_ipaddr = other_proto[interface_side].r_and_q[k].rt->src_ipaddr;
				src_netmask = other_proto[interface_side].r_and_q[k].rt->src_netmask;
						 
				if( ((ntohl(ip->nl_head.ipv4.destination) & dst_netmask) == (dst_ipaddr & dst_netmask)) && 
					((ntohl(ip->nl_head.ipv4.source) & src_netmask) == (src_ipaddr & src_netmask))
				)
					break;
			}
		}

		if( (k>=0) && (k<MAX_MCPC_DST_STA) )
		{
			if (other_proto[interface_side].r_and_q[k].q->q_len <= gw_ifs.c_other_proto_qlen) 
			{
				pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));

				if(!pkt)
				{
					printf("Gateway-->Function other_proto_Handler: my_malloc2 return NULL\n");
					fflush(stdout);
#ifdef TAP_INTERFACE
					free_ll_queue_element (rqts->interface, *buffer);
#else
					free_ll_queue_element (interface, *buffer);
#endif
					return;
				}

				pkt->fd = fd;

#ifdef TAP_INTERFACE
				memcpy (&(pkt->src_mac_addr [0]), &(rqts->src_mac_addr [0]),6);
				memcpy (&(pkt->dst_mac_addr [0]), &(rqts->dst_mac_addr [0]),6);

				pkt->frame_type = rqts->frame_type;
				pkt->mpls_depth = rqts->mpls_depth;  //MPLS

				memcpy (&(pkt->data [0]), (*buffer)->data, MAX_LL_DATA);

				pkt->length = (*buffer)->size;
				pkt->offset = (*buffer)->offset;
#else
				memcpy (&(pkt->data [0]), (*buffer)->data+(*buffer)->offset, (*buffer)->size);
				pkt->length = (*buffer)->size;
#endif

				q_addt (other_proto[interface_side].r_and_q[k].q, (char *) pkt); 
			}
		}
	}

#ifdef TAP_INTERFACE
	free_ll_queue_element (rqts->interface, *buffer);
#else
	free_ll_queue_element (interface, *buffer);
#endif
}

void other_proto_emit ()
{
    	int i, k;
  	other_proto_pkt_t *pkt;
    	int rc;
	int len;

	//printf("other_proto_emit: AIF=%d, BIF=%d\n", AIF, BIF);
   	if(q_empty (other_proto[AIF].r_and_q[0].q)) 
	{
	} 
	else 
	{
		//while(!q_empty (other_proto[AIF].r_and_q[0].q))  //每次发送都把缓存的所有数据包(UDP)发完,这样能保证TCP和UDP和谐共享令牌桶,TCP主动避让,UDP不丢包!
		{
			if (1) 
				{	
					pkt = (other_proto_pkt_t *) q_deq (other_proto[AIF].r_and_q[0].q);

					if(!pkt)
					{
						printf("Gateway-->Function other_proto_emit: q_deq1 return NULL\n");
						fflush(stdout);
						return;
					}

#ifdef TAP_INTERFACE
					if( pkt->mpls_depth > 0 )  //mpls
					{
						//len = pkt->length;
       				pkt->offset -= SIZE_OF_MPLS_TAG*pkt->mpls_depth;
       	   		pkt->length += SIZE_OF_MPLS_TAG*pkt->mpls_depth;
					}

					if( (0x81==pkt->data[START_OF_FRAME_TYPE]) && (0x00==pkt->data[START_OF_FRAME_TYPE+1]) )  //vlan
					{
						len = pkt->length;
       				pkt->offset -= (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG);
       	   		pkt->length += (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG);
					}
					else
					{
						len = pkt->length;
       				pkt->offset -=SIZE_OF_ETHER_PART;
       	   		pkt->length +=SIZE_OF_ETHER_PART;
					}
					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
#else
					len = pkt->length;
					rc = write(pkt->fd, pkt->data, pkt->length);
#endif

					if (rc > 0) 
					{
       					
						if(other_proto[AIF].r_and_q[0].rt->current_credit - len > other_proto[AIF].r_and_q[0].rt->current_credit)
						{
						}
						else
							other_proto[AIF].r_and_q[0].rt->current_credit -= len;

#ifdef FLOW_CONTROL_THRESH
						if (other_proto[AIF].r_and_q[0].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) 
						{  
							if(other_proto[AIF].r_and_q[0].rt->flow_control - len > other_proto[AIF].r_and_q[0].rt->flow_control)
							{
							}
							else
								other_proto[AIF].r_and_q[0].rt->flow_control -= len;
						}
#endif
					}
				//printf("other_proto_handler.c 392\n");
					if (pkt)
						my_free (pkt);
				} 
				else if (gw_ifs.c_other_proto_xrate_drop == 1) 
				{
					pkt = (other_proto_pkt_t *) q_deq (other_proto[AIF].r_and_q[0].q);
					if (pkt)
						my_free (pkt);
				} 
				else 
				{
					//proposed by gujvjuan
					/*
					pkt = (other_proto_pkt_t *) q_deq (other_proto[AIF].r_and_q[0].q);

#ifdef TAP_INTERFACE
					len = pkt->length;
       				pkt->offset -=SIZE_OF_ETHER_PART;
       	   			pkt->length +=SIZE_OF_ETHER_PART;
					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
#else
					len = pkt->length;
					rc = write(pkt->fd, pkt->data, pkt->length);
#endif
					my_free (pkt);
					*/
				}
			}
		}

		for(k = 0; k < MAX_MCPC_DST_STA; k++)
		{
			if(other_proto[BIF].r_and_q[k].sign == 1)
			{
				if(q_empty (other_proto[BIF].r_and_q[k].q)) 
				{
				} 
				else 
				{
					//while(!q_empty (other_proto[BIF].r_and_q[k].q)) //每次发送都把缓存的所有数据包(UDP)发完,这样能保证TCP和UDP和谐共享令牌桶,TCP主动避让,UDP不丢包!
					{
						if (1/*( ((other_proto_pkt_t *)other_proto[BIF].r_and_q[k].q->q_head->qe_data)->length < other_proto[BIF].r_and_q[k].rt->current_credit) 
#ifdef FLOW_CONTROL_THRESH
							&& ((other_proto[BIF].r_and_q[k].rt->cong_control != FLOW_CONTROL_CONGESTION_CONTROL) ||
							((other_proto[BIF].r_and_q[k].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) &&
							(((other_proto_pkt_t *)other_proto[BIF].r_and_q[k].q->q_head->qe_data)->length < other_proto[BIF].r_and_q[k].rt->flow_control)))
#endif
							*/) 
						{	
							pkt = (other_proto_pkt_t *) q_deq (other_proto[BIF].r_and_q[k].q);

							if(!pkt)
							{
								printf("Gateway-->Function other_proto_emit: q_deq2 return NULL\n");
								fflush(stdout);
								return;
							}

#ifdef TAP_INTERFACE
							if( pkt->mpls_depth > 0 )  //mpls
							{
								//len = pkt->length;
       						pkt->offset -= SIZE_OF_MPLS_TAG*pkt->mpls_depth;
       	   				pkt->length += SIZE_OF_MPLS_TAG*pkt->mpls_depth;
							}				
							if( (0x81==pkt->data[START_OF_FRAME_TYPE]) && (0x00==pkt->data[START_OF_FRAME_TYPE+1]) )  //vlan
							{
								len = pkt->length;
       						pkt->offset -= (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG);
       	   				pkt->length += (SIZE_OF_ETHER_PART + SIZE_OF_8021Q_TAG);
							}
							else
							{
								len = pkt->length;
       						pkt->offset -=SIZE_OF_ETHER_PART;
       	   				pkt->length +=SIZE_OF_ETHER_PART;
							}
							rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
#else
							len = pkt->length;
							rc = write(pkt->fd, pkt->data, pkt->length);
#endif

							if (rc > 0) 
							{
       							
								if(other_proto[BIF].r_and_q[k].rt->current_credit - len > other_proto[BIF].r_and_q[k].rt->current_credit )
								{
								}
								else
									other_proto[BIF].r_and_q[k].rt->current_credit -= len;

#ifdef FLOW_CONTROL_THRESH
				    			if (other_proto[BIF].r_and_q[k].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) 
								{
										if(other_proto[BIF].r_and_q[k].rt->flow_control - len > other_proto[BIF].r_and_q[k].rt->flow_control )
										{
										}
										else
											other_proto[BIF].r_and_q[k].rt->flow_control -= len;

								}
#endif
							}
						//printf("other_proto_handler.c 497\n");
							if (pkt)
								my_free (pkt);
						} 
						else if (gw_ifs.c_other_proto_xrate_drop == 1) 
						{
							pkt = (other_proto_pkt_t *) q_deq (other_proto[BIF].r_and_q[k].q);
							if (pkt)
								my_free (pkt);
						} 
						else 
						{
							/*
							pkt = (other_proto_pkt_t *) q_deq (other_proto[BIF].r_and_q[k].q);
#ifdef TAP_INTERFACE				
							len = pkt->length;
       						pkt->offset -=SIZE_OF_ETHER_PART;
       	   					pkt->length +=SIZE_OF_ETHER_PART;
							rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
#else
							len = pkt->length;
							rc = write(pkt->fd, pkt->data, pkt->length);
#endif
							my_free (pkt);
							*/
						}
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

	if (other_proto_non_ip[interface_side].r_and_q[0].q->q_len <= gw_ifs.c_other_proto_qlen) 
	{
		pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));
			if(!pkt)
			{
				printf("Gateway-->Function other_proto_non_ip_Handler: my_malloc return NULL\n");
				fflush(stdout);
				free_ll_queue_element (rqts->interface, *buffer);
				return;
			}
		pkt->fd = fd;
		memcpy (&(pkt->data [0]), (*buffer)->data, (*buffer)->size);
		pkt->length = (*buffer)->size;
		pkt->offset = (*buffer)->offset;
		q_addt(other_proto_non_ip[interface_side].r_and_q[0].q, (char *) pkt);
	}	
        
	free_ll_queue_element (rqts->interface, *buffer);
}

void other_proto_non_ip_emit ()
{
	int j = 0;
  	other_proto_pkt_t *pkt;
    int rc;

	for(j=0; j<2; j++)
	{
		if (q_empty (other_proto_non_ip[j].r_and_q[0].q)) 
		{
		} 
		else 
		{
			//while(!q_empty (other_proto_non_ip[j].r_and_q[0].q))    //好处同上
			{
				if (1/*( ((other_proto_pkt_t *)other_proto_non_ip[j].r_and_q[0].q->q_head->qe_data)->length < other_proto_non_ip[j].r_and_q[0].rt->current_credit) 
#ifdef FLOW_CONTROL_THRESH
	   					&& ((other_proto_non_ip[j].r_and_q[0].rt->cong_control != FLOW_CONTROL_CONGESTION_CONTROL) ||
		    	       ((other_proto_non_ip[j].r_and_q[0].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) &&
						(((other_proto_pkt_t *)other_proto_non_ip[j].r_and_q[0].q->q_head->qe_data)->length < other_proto_non_ip[j].r_and_q[0].rt->flow_control)))
#endif
					*/) 
				{	
					pkt = (other_proto_pkt_t *) q_deq (other_proto_non_ip[j].r_and_q[0].q);
					if(!pkt)
					{
						printf("Gateway-->Function other_proto_non_ip_emit: q_deq return NULL\n");
						fflush(stdout);
						return;
					}
			
					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);

					if (rc > 0) 
					{       					
						if(other_proto_non_ip[j].r_and_q[0].rt->current_credit - rc > other_proto_non_ip[j].r_and_q[0].rt->current_credit)
						{
						}
						else
							other_proto_non_ip[j].r_and_q[0].rt->current_credit -= rc;

#ifdef FLOW_CONTROL_THRESH
						if (other_proto_non_ip[j].r_and_q[0].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) 
						{
       						
							if(other_proto_non_ip[j].r_and_q[0].rt->flow_control - rc > other_proto_non_ip[j].r_and_q[0].rt->flow_control)
							{
							}
							else
								other_proto_non_ip[j].r_and_q[0].rt->flow_control -= rc;
						}
#endif
					}

					if (pkt)
						my_free (pkt);		
				} 
				else if (gw_ifs.c_other_proto_xrate_drop == 1) 
				{
					pkt = (other_proto_pkt_t *) q_deq (other_proto_non_ip[j].r_and_q[0].q);
					if (pkt)
						my_free (pkt);	
				} 
				else 
				{	
					/*
					pkt = (other_proto_pkt_t *) q_deq (other_proto_non_ip[j].r_and_q[0].q);			
					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
					my_free (pkt);
					*/
				}
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

    if (other_proto_ipv6 [interface_side].r_and_q[0].q->q_len <= gw_ifs.c_other_proto_qlen) 
	{
        pkt =  (other_proto_pkt_t *) my_malloc (sizeof (other_proto_pkt_t));
			if(!pkt)
			{
				printf("Gateway-->Function other_proto_ipv6_Handler: my_malloc return NULL\n");
				fflush(stdout);
				free_ll_queue_element (rqts->interface, *buffer);
				return;
			}
		pkt->fd = fd;
        memcpy (&(pkt->data [0]), (*buffer)->data, (*buffer)->size);
        pkt->length = (*buffer)->size;
        pkt->offset = (*buffer)->offset;
        q_addt (other_proto_ipv6[interface_side].r_and_q[0].q, (char *) pkt);
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
        if (q_empty (other_proto_ipv6[j].r_and_q[0].q)) 
		{
        }
		else 
		{
			//while(!q_empty (other_proto_ipv6[j].r_and_q[0].q))
			{
				if (1/*( ((other_proto_pkt_t *)other_proto_ipv6[j].r_and_q[0].q->q_head->qe_data)->length < other_proto_ipv6[j].r_and_q[0].rt->current_credit)
#ifdef FLOW_CONTROL_THRESH
	   				&& ((other_proto_non_ip[j].r_and_q[0].rt->cong_control != FLOW_CONTROL_CONGESTION_CONTROL) ||
		    	       ((other_proto_non_ip[j].r_and_q[0].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) &&
			       (((other_proto_pkt_t *)other_proto_non_ip[j].r_and_q[0].q->q_head->qe_data)->length < other_proto_non_ip[j].r_and_q[0].rt->flow_control)))
#endif
					*/) 
				{
					pkt = (other_proto_pkt_t *) q_deq (other_proto_ipv6[j].r_and_q[0].q);

					if(!pkt)
					{
						printf("Gateway-->Function other_proto_ipv6_emit: q_deq1 return NULL\n");
						fflush(stdout);
						return;
					}

					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);

					if (rc > 0) 
					{						
						if(other_proto_ipv6[j].r_and_q[0].rt->current_credit - rc > other_proto_ipv6[j].r_and_q[0].rt->current_credit)
						{
						}
						else
							other_proto_ipv6[j].r_and_q[0].rt->current_credit -= rc;

#ifdef FLOW_CONTROL_THRESH
						if (other_proto_non_ip[j].r_and_q[0].rt->cong_control == FLOW_CONTROL_CONGESTION_CONTROL) 
						{
							if(other_proto_ipv6[j].r_and_q[0].rt->flow_control - rc > other_proto_ipv6[j].r_and_q[0].rt->flow_control)
							{
							}	
							else
								other_proto_ipv6[j].r_and_q[0].rt->flow_control -= rc;
						}
#endif
					}

                			if (pkt)
						my_free (pkt);	
				} 
				else if (gw_ifs.c_other_proto_xrate_drop == 1) 
				{
					pkt = (other_proto_pkt_t *) q_deq (other_proto_ipv6[j].r_and_q[0].q);
					if(!pkt)
					{
						printf("Gateway-->Function other_proto_ipv6_emit: q_deq2 return NULL\n");
						fflush(stdout);
						return;
					}
					if (pkt)
						my_free (pkt);	
				}
				else 
				{
					/*
					pkt = (other_proto_pkt_t *) q_deq (other_proto_ipv6[j].r_and_q[0].q);
					rc = ll_tap_qk_send (pkt->fd, pkt->data + pkt->offset, pkt->length);
					my_free (pkt);
					*/
				}
			}
        }
    }
}
#endif /* TAP_INTERFACE */

#endif /* GATEWAY */
