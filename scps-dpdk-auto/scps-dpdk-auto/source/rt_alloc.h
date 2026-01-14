#ifdef SCPS_ROUTE_H
#define SCPS_ROUTE_H  
#endif /* SCPS_ROUTE_H */

#include "scps_ip.h"
#include "scpsudp.h"
#include "scpstp.h"
#include "net_types.h"
#include "../include/scps.h"
 
void route_initialize (void);
void init_default_routes (void);
int route_create_default (int rate, int mtu, int smtu);
int route_create (int rate, int mtu, int smtu);
int route_rt_add (route *this_route);
int route_rt_delete (route *this_route);
void  route_rt_avail (route *this_route);
void  route_rt_unavail (route *this_route);

route * route_rt_lookup (uint32_t src_ipaddr, uint32_t dst_ipaddr, unsigned short src_port, unsigned short dst_port, unsigned char proto_id, unsigned char dscp, int lan_or_wan);  
route * route_rt_lookup_ipport_both (uint32_t src_ipaddr, uint32_t dst_ipaddr, unsigned short src_port, unsigned short dst_port);
route * route_rt_lookup_ipport_dst (uint32_t dst_ipaddr, unsigned short dst_port);
route * route_rt_lookup_ip_both (uint32_t src_ipaddr, uint32_t dst_ipaddr);
route * route_rt_lookup_ip_dst (uint32_t dst_ipaddr);
route * route_rt_lookup_ip_dscp (uint32_t dst_ipaddr, unsigned char dscp);
route * route_rt_lookup_lan_wan_dscp (uint32_t lan_or_wan, unsigned char dscp);

int     route_add (uint32_t src_ipddr, uint32_t src_netmask,
                   uint32_t dst_ipddr, uint32_t dst_netmask, unsigned short src_lowport,
		   unsigned short src_higport, unsigned short dst_lowport,
		   unsigned short dst_higport, int rate, int min_rate, int flow_control, int mtu, int smtu,
		   int protocol_id, int dscp, int lan_wan, int cong_control);
int     route_modify (int route_id, int tag, int value);
int     route_delete (int route_id);
int     route_avail (int route_id);
int     route_unavail (int route_id);

route * route_rt_lookup_s (tp_Socket *s);


#define ROUTE_ADD_IPPORT_BOTH(src_ipddr,src_netmask,dst_ipddr,dst_netmask, \
			      src_lowport, src_higport, dst_lowport, dst_higport, \
			      rate, min_rate, flow_control, mtu, smtu, protocol_id, dscp, lan_wan, cong_control) \
	route_add (src_ipddr, src_netmask, dst_ipddr, dst_netmask, \
		    src_lowport, src_higport, dst_lowport, dst_higport, \
		    rate, min_rate, flow_control, mtu, smtu, protocol_id, dscp, lan_wan, cong_control)

#define ROUTE_ADD_IP_BOTH(src_ipddr,src_netmask,dst_ipddr,dst_netmask,rate, \
				min_rate, flow_control, mtu,smtu, protocol_id, dscp, cong_control) \
	route_add (src_ipddr, src_netmask, dst_ipddr, dst_netmask, \
		   0, 0, 0, 0, rate, min_rate, flow_control, mtu, smtu, protocol_id, dscp, cong_control)

#define ROUTE_ADD_IPPORT_DST(dst_ipddr,dst_netmask,dst_lowport,dst_higport, \
		    	     rate,min_rate, flow_control, mtu,smtu, protocol_id, dscp, cong_control) \
	route_add (0, 0xffffffff, dst_ipddr, dst_netmask, \
		   0, 0, dst_lowport, dst_higport, rate, min_rate, flow_control, mtu, smtu, \
		   protocol_id, dscp, cong_control)

#define ROUTE_ADD_IP_DST(dst_ipddr,dst_netmask,rate,min_rate, flow_control, mtu,smtu,protocol_id, \
			 dscp, cong_control) \
	route_add (0, 0xffffffff, dst_ipddr, dst_netmask, 0, 0, 0, 0,rate, min_rate, flow_control, mtu, \
		   smtu, protocol_id, dscp, cong_control)

