#ifndef _scps_np_h
#define _scps_np_h

#include "../include/scps.h"
#include "scpstp.h"
/* #include "mib.h" */
#include "buffer.h"
#include "q.h"
/*
*       Header file for SCPS NP protocol
*/

#define MIN_NPHDR_LEN  5
#define MAX_NPHDR_LEN  45

#define PATHLEN     7		/* fixed num of octets */
/* indices to templ->pointers array */
#define TS_PTR		0	/* points to TS in template header */
#define MC_PTR		2	/* this template address(es) is multicast */
#define BQOS_PTR	3	/* points to BQOS in template header */
#define SA_PTR		4	/* this template has a source addr present */

typedef enum
  {
    es_basic = 0x102,
    es_extended = 0x104,
    es_IP = 0x105,
    es_IPv6 = 0x110,
    path_basic = 0x002,
    path_extended = 0x004,
    free_flyer = 0x01,
    none = 0x0
  }
np_addr_type;

typedef struct
  {
    uint32_t ip;
    uint32_t np;
  }
ip_np_entry;

/* macro to fill in defaults */
/* to be used as follows:  my_rqts.bqos.precedence = NP_DEFAULT(bqos.precedence); */
#define NP_DEFAULT(p) (scps_np_default.(p))

/* SCPS NP API function prototypes */

/* Function to initialize the np protocol.  Should be called exactly once. */
void scps_np_init ();

uint32_t get_next_hop (uint32_t dst_addr);

uint32_t get_path_dst (uint32_t path_addr, short *is_mc);
short get_next_mhop (uint32_t dst_addr, uint32_t * next_hop);
short get_path_mhop (uint32_t dst_addr, uint32_t * next_hop);

#define MAX_ADDR        100
ip_np_entry npAddrConvTable[MAX_ADDR];

typedef struct _np_internals
  {
    queue *procprec_q[MAX_PREC];
    queue *sendprec_q[MAX_PREC];
    BOOL send_pkts;
    BOOL proc_pkts;
    short send_prec;
    short proc_prec;
    scps_np_addr rec_srcs[MAX_ADDR];
    int total_proc_pkts;
    int total_send_pkts;
  }
np_Internals;

typedef struct _np_prec_struct
  {
    byte dg[MAX_MTU];
    BOOL is_mc;
    int len;
    union
      {
	scps_np_addr next_hop;
	uint32_t next_mhop[MAX_ADDR];
      }
    next_addrs;
    uint32_t src_addr;		/* update to accomodate MC */

  }
NP_dg;

typedef struct _npNextHopEntry
  {
    uint32_t dst;
    uint32_t nexthop;
  }
npNextHopEntry;

typedef struct _npMultiNextHopEntry
  {
    uint32_t dst;
    uint32_t nexthop[MAX_NUM_IF + 1];
  }
npMultiNextHopEntry;

typedef struct _npPathEntry
  {
    uint32_t addr;
    uint32_t src;
    uint32_t dst[MAX_NUM_IF + 1];
  }
npPathEntry;

#endif /* _scps_np_h */
