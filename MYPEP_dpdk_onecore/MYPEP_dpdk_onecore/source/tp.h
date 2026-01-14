#ifndef __tp_h__
#define __tp_h__

#include "buffer.h"

#define TP_MAX_WINSHIFT	14	/* maximum window scale value */
#define TP_MAXWIN		(65535)	/* maximum unscaled window value */

extern const uint32_t tp_TICK;
extern const uint32_t tp_ACKDELAY;
extern const uint32_t tp_ACKFLOOR;
extern const uint32_t tp_RTOMIN;
extern const uint32_t tp_RETRANSMITTIME;
extern const uint32_t tp_PERSISTTIME;
extern const uint32_t tp_TIMEOUT;
extern const uint32_t tp_LONGTIMEOUT;
extern const uint32_t tp_2MSLTIMEOUT;
extern const uint32_t tp_KATIMEOUT;
extern const uint32_t tp_BIGTIME;
extern const uint32_t HASH_SIZE;
extern const uint32_t BUFFER_SIZE;

#define SCPS_IPIP	4	/* protocol id for IPIP encapsulation */
#define SCPSCTP 105		/* protocol id for compressed TP */
#ifdef ENCAP_DIVERT
#define SCPSTP  6		/* protocol id for uncompressed TP */
#define SCPSUDP 17		/* protocol id for SCPS UDP */
#else /* ENCAP_DIVERT */
#define SCPSTP  106		/* protocol id for uncompressed TP */
#define SCPSUDP 117		/* protocol id for SCPS UDP */
#endif /* ENCAP_DIVERT */
#define SCPSNP  118
#define SCMP    2		/* protocol id for SCPS Control Message Protocol */

#endif /* __tp_h__ */
