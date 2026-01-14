#ifndef scpssp
#define scpssp
#define MAX_TPDATA_LEN    2048	/* Maximum TP data packet length 
				   (excluding the tcp_PseudoHeader)        */

#ifdef USESCPSNP
#include "net_types.h"
#endif /* USESCPSNP */


/*  Quality-of-Service flags for scps_sp to be found in the 4 low-order
    bits of the clear header  */
#define CONFIDENTIALITY  0x01
#define AUTHENTICATION   0x02
#define SECURITY_LABEL   0x04
#define INTEGRITY        0x08


/* Bitflags indicating presence or absence of optional fields within the
   protected header.
*/
#define ICV_APPEND     0x01
#define PADDING        0x02
#define ENCAPS_NP_ADDR 0x04
#define SEC_LABEL      0x08

/* Note: The optional ICV is appended to the pdu.
*/

/* Error conditions:
*/
enum ERRORS
  {
    MEM_ALLOC_FAILED,
    DATA_OVERFLOW,
    INTEGRITY_CHECK_FAILED,
    AUTHENTICATION_FAILED,
    ERROR_ACCESSING_SA_FILE,
    SA_NOT_FOUND,
    CORRUPTED_SP_PDU,
    SECURITY_LABEL_BAD
  };

void log_sp_error (enum ERRORS error);

#define SECURE_GATEWAY_NO_SECURITY	0
#define SECURE_GATEWAY_ON_DEMAND	1
#define SECURE_GATEWAY_STRICT_SECURITY	2

#endif /* scpssp */
