#ifdef TAP_INTERFACE

#include "../include/scps.h"
#include "scps_ip.h"
#include "scpsudp.h"
#include "tp_debug.h"  // Included for LOG_PACKET in case DEBUG_TCPTRACE is defined.
#include "bbr.h"
#ifdef LINUX
#include <linux/if_tun.h>
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <net/if_tap.h>
#endif

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: tap.h,v $ -- $Revision: 1.5 $\n";
#endif
typedef struct Node 
{
    uint8_t *data;
    uint32_t time;
    uint32_t timesample;
    uint32_t sock;
    uint32_t size;
    struct Node *next;
} Node;

typedef struct Data
{
    uint8_t *data;
    uint32_t time;
    uint32_t timesample;
    uint32_t sock;
    uint32_t size;
}Data;

int tap_open (char dev []);

int ll_tap_qk_send (int tun_fd, unsigned char *data, int len);

int ll_tap_send  (struct _interface *interface, uint32_t remote_internet_addr,
                  int protocol, int data_len, struct msghdr *my_msg,
                  route *a_route, scps_np_rqts *rqts);

int tap_ind (struct _interface *interface, struct _ll_queue_element **buffer,
             int max_len, int *offset, scps_np_rqts *rqts);

void gateway_tap_cleanup(int a);

void gateway_tap_rules (void);

#endif /* TAP_INTERFACE */
