#ifndef __scps_constants_h__
#define __scps_constants_h__

const uint32_t INTER_PACKET_GAP = 375;     /* Inter packet (start-to-start)gap (375 usec) */

const uint32_t tp_TICK = 100000;	/* 100 ms tick (273/4096) */

const uint32_t tp_ACKDELAY = 200000;	/* 200ms ack delay  */
const uint32_t tp_ACKFLOOR = 50000;	/* Don't ack more often than this */
const uint32_t tp_RTOMIN = 250000;		/* 250 msec min rto */
const uint32_t tp_RTOMAX = 64000000;		/* 64 sec max rto */
const uint32_t tp_RETRANSMITTIME = 250000;		/* rexmit call interval */
const uint32_t tp_PERSISTTIME = 1000000;	/* us until probe sent */
const uint32_t tp_TIMEOUT = 320;	/* max rexmits during a connection */
const uint32_t tp_LONGTIMEOUT = 320;	/* max rexmits for opens */
const uint32_t tp_MAXPERSIST_CTR  = 32;	/* max persistance during a connection */
const uint32_t tp_RTOPERSIST_MAX  = 60 * 1000 * 1000;	/* max persistance during a connection */
#ifdef GATEWAY
const uint32_t tp_2MSLTIMEOUT = 10;        /* 2 MSL in seconds */
#else /* GATEWAY */
const uint32_t tp_2MSLTIMEOUT = 60;        /* 2 MSL in seconds */
#endif /* GATEWAY */
const uint32_t tp_KATIMEOUT   = 60;    /* KeepAlive Timer */

const uint32_t BETS_RECEIVE_TIMEOUT = 0x100001;
const unsigned int BETS_MAXIMUM_TRANSMISSIONS = 20;

const unsigned int DEFAULT_ACK_FREQ = 1;	/* Default Ack behavior */

const uint32_t tp_BIGTIME = 0x0fffffff;	/* A large value of time */
const uint32_t HASH_SIZE = 100000;		/* bytes between hash marks */

const uint32_t BUFFER_SIZE = 32768;

const unsigned short tp_MFX_RETRANSMISSION_COUNT = 3;	/* cycle seg n times */
const uint32_t MFX_RETRANSMISSION_INTERLEAVE = 10000;	/* 10 ms */
#endif /* __scps_constants_h__ */
