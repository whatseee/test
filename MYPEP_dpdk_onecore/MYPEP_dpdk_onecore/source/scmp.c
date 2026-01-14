/*
 * SCPS Control Message Protocol - an ICMP clone
 */
#include "../include/scps.h"
#include "scpstp.h"

#ifdef SCMP_TEST
#include "scmp.h"
#include "scmp_var.h"
#endif /* SCMP_TEST */

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: scmp.c,v $ -- $Revision: 1.9 $\n";
#endif

extern route *def_route;

#ifdef NOT_DEFINED
/* Note: fix scmp_handler to use rqts structure */
void
scmp_Handler (scps_np_rqts * rqts, int len, tp_Header * tp)
{
  u_char link_state;
  in_Header *ip;

  link_state = *((u_char *) ip + in_GetHdrlenBytes (ip));

  if (link_state)
    def_route->flags |= RT_LINK_AVAIL;
  else
    def_route->flags &= ~RT_LINK_AVAIL;
}
#endif /* NOT_DEFINED */

#ifdef SCMP_TEST
void
scmp_error (oip, type, code, dest)
     in_Header *oip;
     int type, code;
     uint32_t dest;
{
  in_Header *nip;
  word oiplen = in_GetHdrlenBytes (oip);
  struct icmp *icp;
  word icmplen, len;

#ifdef ICMPPRINTFS
  if (icmpprintfs)
    printf ("icmp_error(%x, %d, %d)\n", oip, type, code);
#endif /* ICMPPRINTFS */

  if (type != ICMP_REDIRECT)
    icmpstat.icps_error++;

  if ((in_GetProtocol (oip) == SCMP) &&
      (type != ICMP_REDIRECT) &&
      (!ICMP_INFOTYPE (((struct icmp *) ((caddr_t) oip + oiplen))->icmp_type)))
    {
      icmpstat.icps_oldicmp++;
      return;
    }

  icmplen = oiplen + min (8, (oip->length - oiplen));
  len = icmplen + ICMP_MINLEN;

  icp = (struct icmp *) (out_buf + oiplen);

  if ((u_int) type > ICMP_MAXTYPE)
    {
      printf ("PANIC:  scmp_error\n");
      exit (1);
    }

  icmpstat.icps_outhist[type]++;
  icp->icmp_type = type;
  if (type == ICMP_REDIRECT)
    {
      icp->icmp_gw_addr = dest;
    }
  else
    {
      icp->icmp_void = 0;
      /* Overlay icmp_void field */
      if (type == ICMP_PARAMPROB)
	{
	  icp->icmp_pptr = code;
	  code = 0;
	}
      else if ((type == ICMP_UNREACH) &&
	       (code == ICMP_UNREACH_NEEDFRAG) &&
	       destifp)
	{
	  icp->icmp_nextmtu = htons (destifp->if_mtu);
	}
    }
  icp->icmp_code = code;

  bcopy ((caddr_t) oip, (caddr_t) & (icp->icmp_ip), icmplen);
  bcopy ((caddr_t) oip, (caddr_t) & out_data, sizeof (in_Header));

  len += sizeof (in_Header);
  nip = (in_Header *) out_data;
  nip->length = len;
  nip->vht = 0x4500;
  nip->ttlProtocol = (250 << 8) + SCMP;

  icmp_reflect (nip);
}

#endif /* SCMP_TEST */
