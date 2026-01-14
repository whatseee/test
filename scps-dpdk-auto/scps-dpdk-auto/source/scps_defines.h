#ifndef scps_defines_h
#define scps_defines_h

#include "../include/scps.h"
/* #define MAX_ETH    0x5dc */
#define MAX_NP_TOTAL_LEN 0x1fff

#ifdef GATEWAY_SELECT
#ifdef GATEWAY_LARGER 
#define MAX_SCPS_SOCKET   512	/* must be same size of (scps_fd_set) */
#else /* GATEWAY_LARGER  */
#define MAX_SCPS_SOCKET   256	/* must be same size of (scps_fd_set) */
#endif /* GATEWAY_LARGER  */
#else /* GATEWAY_SELECT */
#define MAX_SCPS_SOCKET         32	/* based on sizeof (int) */
#endif /* GATEWAY_SELECT */

#define TRUE        1
#define true        1
#define FALSE       0
#define false       0
#ifndef NULL
#define NULL        0		/* An empty value */
#endif /* NULL */
#define NIL         0		/* The dist */


#define DEF_PREC  0
#define DEF_IPPREC 2
#define USE_DEF_PREC    29
#define MAX_PREC    15
#define MAX_TS_DEFINES 4
#define LOOP_CONTROL		TRUE
#define DEFAULT_HOP_COUNT	10
#define MAX_NUM_GROUPS		10
#define MAX_NUM_IF		3
#define NIL     0
#define PATHMASK 0xfffff3f7	/* doesn't have SelfID, BitFd conts, & byte 4 */
#define VPI     0x80
#define RTG_MASK 0xf
#define CKSUM     0x1
#define EXPATH3   0x2		/* Ext path has src addr =0, exp addr=1 bits */
#define ES3       0x42
#define FFLYER   0x1
#define OTHER2    0xb		/* Ext path & ES addr have same 2nd octet */
#define OCTET3   0x42		/* check for Src Addr & Exp Addr fields */

#define SUFF_CL_SYNC   TRUE

#define CKSUM_MASK     0x1
#define DA_MASK        0x2
#define SID_MASK       0x4
#define BFC1_MASK      0x8
#define BFC_MASK       0x80
#define SA_MASK        0x40
#define HP_MASK        0x20
#define TS_MASK        0x18
#define QOS_MASK       0x4
#define ExADD_MASK     0x2
#define INT_DEL_MASK   0x10
#define ExQOS_MASK     0x20
#define IPv6_MASK      0x40

/* don't use about defines; use enumerated type instead: */
#ifdef ENCAP_DIVERT
typedef enum
  {
    TBD0,
    SCMP,			/* SCPS Control Message Protocol */
    TBD2,
    TBD3,
    SCPSCTCP,			/* SCPS Compressed Transport Protocol */
/* SCPSUDP, *//* SCPS User Datagram Protocol */
    SCPSTP = 6,
    TBD7,
    SP,				/* SCPS Security Protocol */
    SP3,			/* Secure Data Network Systems Security Protocol 3 */
    IPV6AUTH,			/* Speculative IPv6 authorization exchange */
    IPV6ESP,			/* Speculative IPv6 telepathic exchange */
    TBD12,
    TBD13,
    TBD14,
    TBD15,
    SCPSUDP = 17,
    SCPSCTP = 105,
    SCPSNP = 118
  }
scps_tpid;
#else /* ENCAP_DIVERT */
typedef enum
  {
    TBD0,
    SCMP,			/* SCPS Control Message Protocol */
    TBD2,
    TBD3,
    SCPSCTCP,			/* SCPS Compressed Transport Protocol */
/* SCPSUDP, *//* SCPS User Datagram Protocol */
    TBD6 = 6,
    TBD7,
    SP,				/* SCPS Security Protocol */
    SP3,			/* Secure Data Network Systems Security Protocol 3 */
    IPV6AUTH,			/* Speculative IPv6 authorization exchange */
    IPV6ESP,			/* Speculative IPv6 telepathic exchange */
    TBD12,
    TBD13,
    TBD14,
    TBD15,
    SCPSCTP = 105,
    SCPSTP = 106,
    SCPSUDP = 117,
    SCPSNP = 118
  }
scps_tpid;
#endif /* ENCAP_DIVERT */

/* Move these to scps.h, they need to be there */

/* SCPS Protocol IDs */
#define NP	0		/*  SCPS Network Protocol */
#define SCMP	1		/*  SCPS Control Message Protocol */
#define TBD2	2
#define TBD3	3
#define SCPS_IPIP	4	/* Protocol id for IPIP encapsulation */
#define SCPSCTP 105		/* SCPS Compressed TCP Headers   */

#ifdef ENCAP_DIVERT
#define SCPSTP  6		/* SCPS TCP */
#define SCPSUDP 17		/* SCPS User Datagram Protocol   */
#else /* ENCAP_DIVERT */
#define SCPSTP  106		/* SCPS TP */
#define SCPSUDP 117		/* SCPS User Datagram Protocol   */
#endif /* ENCAP_DIVERT */

#define SP	7		/* SCPS Security Protocol */
#define TBD8	8
#define SP3	9
#define IPV6AUTH 10
#define IPV6ESP 11

#define PROTO_MAX 256

#define DEFAULT_RATE            2000000

#ifdef GATEWAY
#define GATEWAY_DEFAULT_RATE            2000000
#define GATEWAY_DEFAULT_BUFFER          32768
#endif /* GATEWAY */

#endif /* scps_defines_h */
