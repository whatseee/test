#ifndef OTHER_PROTO_HANDLER_H
#define OTHER_PROTO_HANDLER_H 

#include "scps.h"
#include "scpstp.h"
#include "scpserrno.h"
#include "tp_debug.h"
#include "gateway.h"
#include <stdio.h>
#include <math.h>


#ifdef SCPSSP
#include "scps_sp.h"
#endif /* SCPSSP */

#include "rt_alloc.h"

#include "scps_ip.h"
#include "scps_np.h"
#include "rs_config.h"

#define AIF	0
#define BIF	1

#define OTHER_PROTO_QLEN_DEFAULT	5

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: other_proto_handler.h,v $ -- $Revision: 1.4 $\n";
#endif

typedef
struct other_proto_pkt {
   unsigned char data[MAX_LL_DATA];
   int length;
   int offset;
   int fd;
   unsigned char src_mac_addr [6];
   unsigned char dst_mac_addr [6];
   unsigned short frame_type;
} other_proto_pkt_t;


typedef
struct other_pkt_q {
   queue *q;
   route *rt;
   route *def_rt;
} other_proto_q_t;

/*
 * Handler for incoming TP packets.
 */

void
other_proto_Handler (
struct _interface *interface,
struct _ll_queue_element **buffer,
int max_len,
int *offset,
scps_np_rqts *rqts,
unsigned char proto);

void other_proto_non_ip_Handler (
int interface_side,
struct _ll_queue_element **buffer,
scps_np_rqts *rqts,
int fd);

void other_proto_ipv6_Handler (
int interface_side,
struct _ll_queue_element **buffer,
scps_np_rqts *rqts,
int fd);

void other_proto_init ();
void other_proto_emit ();
void other_proto_non_ip_emit ();
void other_proto_ipv6_emit ();

#endif /* OTHER_PROTO_HANDLER_H */
