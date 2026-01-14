/*
 *
 *	Compressor/decompressor software.
 *	
 */


#ifdef OPT_OLD_COMPRESS 

#define MAX_CONNS	8	/* 2 < MAX_CONNS < 255 */
#define MAX_HDR		64	/* maximum sized TP header */

#define TP_NO_OPTS	5	/* TP header len in 32-bit words 
				   without options */

/* Compressed TP header Control Flags */

/* Octet 1 */
#define MORE 0x8000		/* another octet of control flags follows 
				   this one */
#define TIME 0x4000		/* the SCPS-Echo reply timestamp is included */

#define RECORD 0x2000		/* the SCPS TP record marker is included */

#define PUSH 0x1000		/* the PUSH flag from the TP header */

#define S1   0x0800		/* A 16-bit sequence number change is being 
				   reported.  Sequence number is the least-
				   significant 16 bits of the 32-bit sequence 
				   number.  Mutex with S2. */
#define A2   0x0400		/* A 32-bit acknowledgment number change is being
				   reported.  Mutex with A1.  */
#define A1   0x0200		/* A 16-bit acknowledgment number change is being
				   reported.  Mutex with A2.  */
#define WIN  0x0100		/* A window update is being reported */

/* Octet 2 */
#define NOTACK 0x80		/* ACK flag NOT set in TP header */

#define OPTS 0x40		/* Uncompressed TP options follow the remainder
				   of the compressed TP header elements */
#define PAD 0x20		/* Pad octet is present */

#define URG  0x10		/* The 16-bit urgent flag is included */

#define RST  0x08		/* The RST flag from the TP header */

#define SYN  0x04		/* The SYN flag from the TP header */

#define FIN  0x02		/* The FIN flag from the TP header */

#define S2   0x01		/* A 32-bit sequence number change is being
				   reported.  Mutex with S1.  */

/* Pre-defined control field for resynchronization */
#define RES1 0x84A1		/* control octets for a full-up
				   resync:  more, S1, A2, A1, WIN,
				   RESYNC, PORTS, S2 */

/* Flag value */
#define NEED_RESYNC 1		/* State is/may be hosed */


/* ENCODE encodes a number that is known to be non-zero.  ENCODEZ 
 * checks for zero (since zero has to be encoded in the long, 3 byte
 * form).
 */
#define ENCODE(n) { \
        if ((int)(n) >= 0x10000) { \
                cp[3] = (n); \
		cp[2] = (n) >> 8; \
		cp[1] = (n) >> 16; \
                cp[0] = (n) >> 24; \
                cp += 4; \
        } else { \
		cp[1] = (n); \
		cp[0] = (n) >> 8; \
		cp += 2; \
        } \
}

#define ENCODE2(n) {        \
	cp[1] = (n);        \
	cp[0] = (n) >> 8;   \
	cp += 2 ;           \
	}

#define ENCODE4(n) {        \
	cp[3] = (n);        \
	cp[2] = (n) >> 8;   \
	cp[1] = (n) >> 16;  \
	cp[0] = (n) >> 24;  \
	cp += 4; \
	}

#define ENCODEZ(n) { \
        if ((u_short)(n) >= 256 || (u_short)(n) == 0) { \
                *cp++ = 0; \
                cp[1] = (n); \
                cp[0] = (n) >> 8; \
                cp += 2; \
        } else { \
                *cp++ = (n); \
        } \
}

#define DECODEW(f) { \
	(f) = htons(((cp[0] & 0xff)<<8) | (cp[1] & 0xff)); \
	cp += 2; \
	}

#define DECODE4(f) { \
	(f) = htonl(  ((cp[0]&0xff) << 24)  \
		     +((cp[1]&0xff) << 16)  \
		     +((cp[2]&0xff) << 8)   \
		     +((cp[3]&0xff)));      \
		     cp+= 4;	     \
	}

#define DECODE2(f) { \
	(f) = (htonl((cp[0]<<8) | cp[1]) | ((f) & 0xffff0000)); \
	cp += 2;  \
	}

#endif /* OPT_OLD_COMPRESS */
