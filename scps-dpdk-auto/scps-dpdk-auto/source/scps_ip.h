#ifndef scps_ip_h
#define scps_ip_h

#include "scpstp.h"

int ip_get_template (scps_np_rqts * rqts, ip_template * templ);
int ip_request (tp_Socket * s, struct mbuff *m, uint32_t * bytes_sent);
int ip_trequest (tp_Socket * s, struct mbuff *m, uint32_t * bytes_sent);
int ip_ind (scps_np_rqts * rqts, int length, int *data_offset);

#ifdef IPV6
int ipv6_get_template (scps_np_rqts * rqts, ip_template * templ);
int ipv6_request (tp_Socket * s, struct mbuff *m, longword * bytes_sent);
int ipv6_trequest (tp_Socket * s, struct mbuff *m, longword * bytes_sent);
int ipv6_ind (scps_np_rqts * rqts, int length, int *data_offset);
#endif /* IPV6 */

#endif /* scps_ip_h */
