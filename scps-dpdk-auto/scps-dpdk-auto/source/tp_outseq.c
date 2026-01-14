#include <string.h>
#include "../include/scps.h"
#include "scpstp.h"
#include <sys/types.h>

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: tp_outseq.c,v $ -- $Revision: 1.13 $\n";
#endif

/*
 * Basic TP Out-of-Sequence Processing for 
 * the receiver. There seems to be too much 
 * code here for the functionality it provides... 
 * Collapse it down to about half it's code size!
 * 
 */

int
tp_OutSeq (tp_Socket * s, tp_Header * tp, int tp_len, byte * dp, int dp_len)
{
  	struct mbuff *head, *new;
  	uint32_t temp;
  	int done;

   /* If this is the FIN, just enqueue it on s->Out_Seq */
  	if (tp->flags & tp_FlagFIN) 
	{
   	if ( (s->Out_Seq) && (s->Out_Seq->last) &&
          (((tp_Header *) (s->Out_Seq->last->m_pktdat))->flags & tp_FlagFIN)) 
		{
      		}	 
		else 
		{
      	if (!(new = alloc_mbuff (MT_HEADER))) 
			{
         	return (0);
          		}

         memcpy (new->m_pktdat, tp, tp_len);
         new->m_len = tp_len;
         new->m_plen = dp_len;
         new->m_seq = ntohl (tp->seqnum);
         enq_mbuff (new, s->Out_Seq);

	  		if (dp_len > 0) 
			{
	  			if (cb_cpdatin (s->app_rbuff, dp, dp_len, (ntohl (tp->seqnum) - s->acknum), 0) < 0) 
				{
	  				return (0);
	  			}
          		}
       		}
 
      return (1);
    	}

  	if (!(new = alloc_mbuff (MT_HEADER))) 
	{
      return (0);
  	}

  	memcpy (new->m_pktdat, tp, tp_len);
  	new->m_len = tp_len;
  	new->m_plen = dp_len;
  	new->m_seq = ntohl (tp->seqnum);
  	done = 0;

 	 /*
   * Write_off corresponds to s->acknum, so data 
   * needs to be written with that in mind. We 
   * can use the improved cp_cpdatin() routine
   * and pass it an offset from the s->acknum - 
   * that should do it for us.
   	*/

  	/*
   * If this segment is at the end of the 
   *  out-of-sequence queue, handle it quickly 
   	*/

  	if ((s->Out_Seq->last) && (ntohl (tp->seqnum) > s->Out_Seq->last->m_seq))
    	{
   	/* if (cb_outseqin(s->Out_Seq->last, dp, dp_len, 
         (ntohl(tp->seqnum) - s->Out_Seq->last->m_seq)) < 0) */
      if (cb_cpdatin (s->app_rbuff, dp, dp_len, (ntohl (tp->seqnum) - s->acknum), 0) < 0)
		{
			printf("Gateway-->Function tp_OutSeq: the 5the return! add free_mbuff(new)!\n");
			fflush(stdout);
			free_mbuff(new);
	  		return (0);
		}
    	}
  	else if (cb_cpdatin (s->app_rbuff, dp, dp_len, (ntohl (tp->seqnum) - s->acknum), 0) < 0)
    	{
		printf("Gateway-->Function tp_OutSeq: the 5the return! add free_mbuff(new)!\n");
		fflush(stdout);
		free_mbuff(new);
      return (0);
    	}
  /*
   * We are playing with the out of sequence queue here, 
   * so offset needs to be relative to the current 
   * position of s->app_rbuff->write_off. This is legal 
   * for mcput (which usually uses an absolute offset)
   */

  if (tp->flags & tp_FlagEOR)
    {
      /* Build and insert the Record Boundary */
      Process_EOR (s, new, 1);
      s->sockFlags &= ~tp_FlagEOR;
    }

  if (mcput (new, s->app_rbuff->write_head,
	     (ntohl (tp->seqnum) - s->acknum), dp_len, 3) < 0)
    {
      /* Log a stat to record mcput failure */
    }

  /*
   * Now, figure out where to stick the new mbuffer within foutseq,
   * collapsing contiguous links where ever possible.
   */

  if ((head = s->Out_Seq->start))
    {
      /* There is an existing chain, we are doing an insertion */
      while (!(done))
	{
	  if (head->m_seq == new->m_seq)
	    {
	      free_mbuff (new);
	      return (1);
	    }
	  if (((head->m_seq >= new->m_seq) &&
	       (head->m_seq <= new->m_seq + new->m_plen))
	      || (head->m_seq == (new->m_seq + new->m_plen + 1)))
	    {
	      /* This overlaps the begining of head or
	       * head follows immediately after new:
	       * prepend new to head 
	       */
if (new ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG new PARENT 1 is on the my_free list\n");
}
if (head ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG head PARENT 1 is on the my_free list\n");
}
      BUG_HUNT (s);
	      mb_merge_cti (new, head);
			//mb_merge (new, head);
      BUG_HUNT (s);
      VERIFY_BUFFER_HOLES (s->send_buff);
      SCRUB_HOLES (s);

	      if (s->Out_Seq->start == head)
		s->Out_Seq->start = new;
	      done = 1;
	    }
	  else if (((head->m_seq < new->m_seq) &&
		    ((head->m_seq + head->m_plen) >= new->m_seq))
		   || (new->m_seq == (head->m_seq + head->m_plen + 1)))
	    {
	      /* 
	       * This starts within head or the
	       * new segment follows immediately 
	       * after head: 
	       * append new to head 
	       */
if (new ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG new PARENT 2 is on the my_free list\n");
}
if (head ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG head PARENT 2 is on the my_free list\n");
}
      BUG_HUNT (s);
			mb_merge_cti (head, new);
	      //mb_merge (head, new);
      BUG_HUNT (s);
      VERIFY_BUFFER_HOLES (s->send_buff);
      SCRUB_HOLES (s);

	      done = 1;
	    }
	  else if( (new->m_seq < head->m_seq) && (head->m_seq - new->m_seq < 0x20000000))  //YGX
	    {
	      /* Insert before current */
	      mb_insert (s->Out_Seq, head, new);

	      if (s->Out_Seq->start == head)
		s->Out_Seq->start = new;

	      /* 
	       * If there's something before this entry on the out of 
	       * sequence queue, then the start of the hole is defined 
	       * by the sum of the sequence number of the data present 
	       * and its length.  This calculates the sequence number
	       * of the first missing octet.  If this value is less 
	       * than the next value to be SNACKed (stored in 
	       * SNACK1_Receive_Hole), then replace it. If there was a 
	       * previous element, but SNACK1_Receive_Hole's sequence 
	       * number was lower that the new sequence number, no change 
	       * needs to be made, because we march through the holes, 
	       * signalling them in ascending sequence number order.  
	       * If there was no previous member on the out of sequence 
	       * queue, then we definitely want to update 
	       * SNACK1_Receive_Hole, and it's easy, since it's value 
	       * is defined by acknum, the first unacknowledged octet.
	       */
	      if (new->m_prev)
		{
		  temp = new->m_prev->m_seq + new->m_prev->m_plen;
		  if (SEQ_LEQ (temp, s->SNACK1_Receive_Hole))
		    s->SNACK1_Receive_Hole = temp;
		}
	      else
		s->SNACK1_Receive_Hole = s->acknum;
	      s->SNACK1_Flags |= SEND_SNACK1;
/* s->sockFlags |= SOCK_ACKNOW; *//* Ack when input queue empty */
	      done = 1;
	    }

	  else if (head->pep_m_next)					//YGX
	{
		if ((head->pep_m_next->m_seq < new->m_seq) && ( new->m_seq - head->pep_m_next->m_seq > 0x80000000))
		{
			mb_insert(s->Out_Seq, head->pep_m_next, new);
			done = 1;
		}
	    
		else head = head->pep_m_next;		
	}
	  else
	    /* Walked off end of Out_Seq queue */
	    {
	      enq_mbuff (new, s->Out_Seq);
/* s->sockFlags |= SOCK_ACKNOW; *//* Ack when input queue empty */
	      /* s->SNACK1_Receive_Hole = new->m_seq; */
	      s->SNACK1_Flags |= SEND_SNACK1;
	      done = 1;
	    }
	}
      return (1);
    }
  else
    {
      enq_mbuff (new, s->Out_Seq);
/* s->sockFlags |= SOCK_ACKNOW; *//* Ack when input queue empty */

      s->SNACK1_Receive_Hole = s->acknum;
      s->SNACK1_Flags |= SEND_SNACK1;
    }
  return (1);
}


int
tp_OutSeq_loop (tp_Socket * s, tp_Header * tp, int tp_len, byte * dp, int dp_len)
{
  	struct mbuff *head, *new;
  	uint32_t temp;
  	int done;

   /* If this is the FIN, just enqueue it on s->Out_Seq */
  	if (tp->flags & tp_FlagFIN) 
	{
   	if ( (s->Out_Seq) && (s->Out_Seq->last) &&
          (((tp_Header *) (s->Out_Seq->last->m_pktdat))->flags & tp_FlagFIN)) 
		{
      		}	 
		else 
		{
      	if (!(new = alloc_mbuff (MT_HEADER))) 
			{
         	return (0);
          		}

         memcpy (new->m_pktdat, tp, tp_len);
         new->m_len = tp_len;
         new->m_plen = dp_len;
         new->m_seq = ntohl (tp->seqnum);
         enq_mbuff (new, s->Out_Seq);

	  		if (dp_len > 0) 
			{
	  			if (cb_cpdatin (s->app_rbuff, dp, dp_len, (ntohl (tp->seqnum) + 0x100000000 - s->acknum), 0) < 0) 
				{
	  				return (0);
	  			}
          		}
       		}
 
      return (1);
    	}

  	if (!(new = alloc_mbuff (MT_HEADER))) 
	{
      return (0);
  	}

  	memcpy (new->m_pktdat, tp, tp_len);
  	new->m_len = tp_len;
  	new->m_plen = dp_len;
  	new->m_seq = ntohl (tp->seqnum);
  	done = 0;

 	 /*
   * Write_off corresponds to s->acknum, so data 
   * needs to be written with that in mind. We 
   * can use the improved cp_cpdatin() routine
   * and pass it an offset from the s->acknum - 
   * that should do it for us.
   	*/

  	/*
   * If this segment is at the end of the 
   *  out-of-sequence queue, handle it quickly 
   	*/

  	if ((s->Out_Seq->last) && (ntohl (tp->seqnum) > s->Out_Seq->last->m_seq))
    	{
   	/* if (cb_outseqin(s->Out_Seq->last, dp, dp_len, 
         (ntohl(tp->seqnum) - s->Out_Seq->last->m_seq)) < 0) */
      if (cb_cpdatin (s->app_rbuff, dp, dp_len, (ntohl (tp->seqnum) + 0x100000000 - s->acknum), 0) < 0)
		{
			printf("Gateway-->Function tp_OutSeq: the 5the return! add free_mbuff(new)!\n");
			fflush(stdout);
			free_mbuff(new);
	  		return (0);
		}
    	}
  	else if (cb_cpdatin (s->app_rbuff, dp, dp_len, (ntohl (tp->seqnum) + 0x100000000 - s->acknum), 0) < 0)
    	{
		printf("Gateway-->Function tp_OutSeq: the 5the return! add free_mbuff(new)!\n");
		fflush(stdout);
		free_mbuff(new);
      return (0);
    	}
  /*
   * We are playing with the out of sequence queue here, 
   * so offset needs to be relative to the current 
   * position of s->app_rbuff->write_off. This is legal 
   * for mcput (which usually uses an absolute offset)
   */

  if (tp->flags & tp_FlagEOR)
    {
      /* Build and insert the Record Boundary */
      Process_EOR (s, new, 1);
      s->sockFlags &= ~tp_FlagEOR;
    }

  if (mcput (new, s->app_rbuff->write_head,
	     (ntohl (tp->seqnum) + 0x100000000 - s->acknum), dp_len, 3) < 0)
    {
      /* Log a stat to record mcput failure */
    }
#if 1
/*********************************YGX***********************************/
if((new->m_seq > 0xFFF00000) && ((new->m_seq + new->m_plen) < 0x100000))
{
	if(outseq_split(s, new) < 1)
	{	
		printf("There is something wrong in outseq_split!");
		return(-1);
	}
	else
		return(1);
}

/*********************************YGX************************************/
#endif
  /*
   * Now, figure out where to stick the new mbuffer within foutseq,
   * collapsing contiguous links where ever possible.
   */

  if ((head = s->Out_Seq->start))
    {
      /* There is an existing chain, we are doing an insertion */
      while (!(done))
	{
	  if (head->m_seq == new->m_seq)
	    {
	      free_mbuff (new);
	      return (1);
	    }
	  if (((head->m_seq >= new->m_seq) &&
	       (head->m_seq <= new->m_seq + new->m_plen))
	      || (head->m_seq == (new->m_seq + new->m_plen + 1)))
	    {
	      /* This overlaps the begining of head or
	       * head follows immediately after new:
	       * prepend new to head 
	       */
if (new ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG new PARENT 1 is on the my_free list\n");
}
if (head ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG head PARENT 1 is on the my_free list\n");
}
      BUG_HUNT (s);
	      mb_merge_cti (new, head);
			//mb_merge (new, head);
      BUG_HUNT (s);
      VERIFY_BUFFER_HOLES (s->send_buff);
      SCRUB_HOLES (s);

	      if (s->Out_Seq->start == head)
		s->Out_Seq->start = new;
	      done = 1;
	    }
	  else if (((head->m_seq < new->m_seq) &&
		    ((head->m_seq + head->m_plen) >= new->m_seq))
		   || (new->m_seq == (head->m_seq + head->m_plen + 1)))
	    {
	      /* 
	       * This starts within head or the
	       * new segment follows immediately 
	       * after head: 
	       * append new to head 
	       */
if (new ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG new PARENT 2 is on the my_free list\n");
}
if (head ->parent == &(sys_memory.fblist) ) {
printf ("DEBUG head PARENT 2 is on the my_free list\n");
}
      BUG_HUNT (s);
			mb_merge_cti (head, new);
	      //mb_merge (head, new);
      BUG_HUNT (s);
      VERIFY_BUFFER_HOLES (s->send_buff);
      SCRUB_HOLES (s);

	      done = 1;
	    }
	  else if( (new->m_seq < head->m_seq) && (head->m_seq - new->m_seq < 0x20000000))  //YGX
	    {
	      /* Insert before current */
	      mb_insert (s->Out_Seq, head, new);

	      if (s->Out_Seq->start == head)
		s->Out_Seq->start = new;

	      /* 
	       * If there's something before this entry on the out of 
	       * sequence queue, then the start of the hole is defined 
	       * by the sum of the sequence number of the data present 
	       * and its length.  This calculates the sequence number
	       * of the first missing octet.  If this value is less 
	       * than the next value to be SNACKed (stored in 
	       * SNACK1_Receive_Hole), then replace it. If there was a 
	       * previous element, but SNACK1_Receive_Hole's sequence 
	       * number was lower that the new sequence number, no change 
	       * needs to be made, because we march through the holes, 
	       * signalling them in ascending sequence number order.  
	       * If there was no previous member on the out of sequence 
	       * queue, then we definitely want to update 
	       * SNACK1_Receive_Hole, and it's easy, since it's value 
	       * is defined by acknum, the first unacknowledged octet.
	       */
	      if (new->m_prev)
		{
		  temp = new->m_prev->m_seq + new->m_prev->m_plen;
		  if (SEQ_LEQ (temp, s->SNACK1_Receive_Hole))
		    s->SNACK1_Receive_Hole = temp;
		}
	      else
		s->SNACK1_Receive_Hole = s->acknum;
	      s->SNACK1_Flags |= SEND_SNACK1;
/* s->sockFlags |= SOCK_ACKNOW; *//* Ack when input queue empty */
	      done = 1;
	    }

	  else if (head->pep_m_next)
	{
		if ((head->pep_m_next->m_seq < new->m_seq) && ( new->m_seq - head->pep_m_next->m_seq > 0x80000000))
		{
			mb_insert(s->Out_Seq, head->pep_m_next, new);
			done = 1;
		}
	    
		else head = head->pep_m_next;		
	}

	  else
	    /* Walked off end of Out_Seq queue */
	    {
	      enq_mbuff (new, s->Out_Seq);
/* s->sockFlags |= SOCK_ACKNOW; *//* Ack when input queue empty */
	      /* s->SNACK1_Receive_Hole = new->m_seq; */
	      s->SNACK1_Flags |= SEND_SNACK1;
	      done = 1;
	    }
	}
      return (1);
    }
  else
    {
      enq_mbuff (new, s->Out_Seq);
/* s->sockFlags |= SOCK_ACKNOW; *//* Ack when input queue empty */

      s->SNACK1_Receive_Hole = s->acknum;
      s->SNACK1_Flags |= SEND_SNACK1;
    }
  return (1);
}

int
outseq_split(tp_Socket * s, struct mbuff *new)
{
	struct mbuff *new1 , *new2, *head;
	int offset = 0, done = 0;

	offset = 0xFFFFFFFF - new->m_seq + 1;
printf("in split:new->m_seq = %d,new->m_plen = %d\n", new->m_seq, new->m_plen);
fflush(stdout);
	copy_mbuff(new1, new);
	copy_mbuff(new2, new);

	new1->m_plen = offset;
	new1->m_ext.len -= offset;

	new2->m_seq = 0;
	new2->m_plen -= offset;
	new2->m_ext.offset += offset;
	
	if(new2->m_ext.offset >= SMCLBYTES)
	{
		new2->m_ext.ext_buf = ((struct mbcluster *)(new2->m_ext.ext_buf))->c_next;
		new2->m_ext.offset -= SMCLBYTES;
	}
	if(head = s->Out_Seq->start)
	{
		while(!done)
		{
			if(head->m_seq == new1->m_seq)
			{
				free(new1);
				free(new2);
				return(1);
			}

			if(((head->m_seq >= new2->m_seq) && (head->m_seq <= new2->m_seq + new2->m_plen)) || (head->m_seq == (new2->m_seq + new2->m_plen + 1)))
			{
				mb_merge_cti(new2, head);
				mb_insert(s->Out_Seq, new2, new1);

				if(s->Out_Seq->start = head)
					s->Out_Seq->start = new1;
				done = 1;
			}
			else if(((head->m_seq <= new1->m_seq) && ((head->m_seq + head->m_plen) >= new1->m_seq)) || (new1->m_seq == (head->m_seq + head->m_plen + 1)))
			{
				mb_merge_cti(head, new1);

				if(head->pep_m_next)
				{
					if(((new2->m_seq <= head->pep_m_next->m_seq) && ((new2->m_seq + new2->m_plen) >= head->pep_m_next->m_seq)) || (head->pep_m_next->m_seq == (new2->m_seq + new2->m_plen + 1)))
					{
						mb_merge_cti(new2, head->pep_m_next);
						done = 1;
					}
					else 
					{
						mb_insert(s->Out_Seq, head->pep_m_next, new2);
						done = 1;
					} 
				}
				else
				{
					enq_mbuff(new2, s->Out_Seq);
					done = 1;
				}
			}
			else if (head->m_seq < 0x20000000)
			{
				mb_insert(s->Out_Seq, head, new2);
				mb_insert(s->Out_Seq, new2, new1);
				done = 1;
			}
			else if(head->pep_m_next)
				head = head->pep_m_next;
			else
			{
				enq_mbuff(new1, s->Out_Seq);
				enq_mbuff(new2, s->Out_Seq);

				done = 1;
			}		
		}	
	}
}
