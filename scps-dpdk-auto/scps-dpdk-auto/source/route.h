#ifdef SCPS_ROUTE_H
#define SCPS_ROUTE_H  
#endif /* SCPS_ROUTE_H */

#include "scps_ip.h"
#include "scpsudp.h"
#include "scpstp.h"
#include "net_types.h"
#include "../include/scps.h"
#include "../include/route.h"
 
void route_initialize (void);
void init_default_routes (void);
int route_create (int rate, int mtu, int smtu);
int route_delete (int this_route);
int route_rt_add (route *this_route);
int route_rt_delete (route *this_route);
void  route_rt_avail (route *this_route);
void  route_rt_unavail (route *this_route);


