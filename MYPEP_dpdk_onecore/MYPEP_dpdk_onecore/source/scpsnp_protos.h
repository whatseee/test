#ifndef _scps_np_protos_h
#define _scps_np_protos_h

/*
*       Header file for SCPS NP prototypes
*/

void purge_proc_queues ();

void purge_send_queues ();

int scmpOutput (uint32_t src_addr, short indx, short type, int prior);

short check_sendqueues (scps_np_template * templ);

int check_procqueues (short prec);

void log_srcs (scps_np_addr src);
/*
 *  scps_np_dg_request is the fully-connectionless API to the scps_np.
 *  The user provides a pointer to a scps_np_requirements structure,
 *  the current timestamp structure (or NULL, at the user's option),
 *  the length of the np user data, and a pointer to the np user data.
 *  scps_np_request returns the number of octets of user data queued 
 *  for transmission, from *  0 - length, or a -1 to indicate an error.  
 *  The error number may be obtained by executing the macro GET_ERRNO(), 
 *  which will return the thread-specific errno.
 */
int
  scps_np_dg_request (
		       tp_Socket * s,
		       scps_ts * ts,
		       short length,
		       struct mbuff *m,
		       u_char th_off);
/*  
 *  scps_np_get_template returns a pre-filled-out scps_np header to the
 *  caller for connection-oriented users that wish to cache the np info
 *  to reduce per-packet processing requirements.  scps_np_user owns
 *  the storage for both the requirements structure and the template
 *  structure.
 */
int scps_np_get_template (scps_np_rqts * rqts,
			  scps_np_template * templ);

/*
 *  scps_np_trequest consumes a user-supplied scps_np_template (as 
 *  created by scps_np_get_template) to expedite the transmission
 *  procedure.  This call is for use by connection-oriented scps_np
 *  users, and accepts (at user's option) already-acquired routing 
 *  information.
 *
 *  The template based transfer accepts a pointer to the template, 
 *  a pointer to the current timestamp structure (or NULL, at the user's 
 *  option), a pointer to the routing structure, the length of the scps_np 
 *  user data, and a pointer to the scps_np user data.  If the requirements 
 *  structure indicates that a timestamp parameter is to accompany this 
 *  datagram and the pointer to the timestamp is NULL, the scps_np will 
 *  insert a properly-formatted timestamp.  If the timestamp is non-NULL, 
 *  the user-supplied timestamp will accompany the datagram.  
 *
 *  scps_np_trequest returns the number of bytes of user data queued for 
 *  transmission, from 0 - length, or a -1 to indicate an error.  The 
 *  error number may be obtained by executing the macro GET_ERRNO(), which 
 *  will return the thread-specific errno.
 */
int scps_np_trequest (tp_Socket * s, scps_ts * ts, route * nproute, uint32_t
		      length, struct mbuff *m, u_char th_off);

/*
 *  scps_np_ind accepts a user-supplied pointer to a scps_np requirements 
 *  structure, a pointer to a buffer for the scps_np user data, and a maximum
 *  length of the data to be returned (typically, the size of the buffer).
 *  If the pointer to the scps_np requirements structure is non-null, the 
 *  address information, transport protocol identifier, source time stamp, 
 *  basic and expanded quality of service and secondary header information 
 *  will be returned.
 *  
 *  The call returns the amount of user data that was received.  If the
 *  amount of data received exceeds the capacity of the user-supplied buffer,
 *  the buffer will be filled to capacity, a thread-specific error number
 *  will be set indicating "message too long" (EMSGSIZE), and -1 will be
 *  returned.  The error number may be obtained by executing the macro
 *  GET_ERRNO(), which will return the thread-specific errno.
 *
 *  Note that within the requirements structure, there is a pointer to a secondary
 *  header buffer.  If this pointer is NULL, no secondary header information
 *  will be returned.  If non-null, the secondary header information will be
 *  written to the indicated buffer, up to the amount indicated by the associated
 *  length parameter in the requirements structure (sec_head_len).  If sec_head_len
 *  is less than the amount of secondary header data present, the remainder will
 *  be silently discarded.  
 */
int scps_np_ind (scps_np_rqts * rqts,
		 short length,
		 int *data_offset);

uint32_t NP_array_to_IP (short cur_index, short addr_len);

uint32_t NP_long_to_IP (scps_np_addr np);

uint32_t IP_to_NP_long (scps_np_addr ip);

uint32_t Convert_to_np (uint32_t addr, short *mc_addr);

void scmpHandler (short indx, int length);

#endif /* _scps_np_protos_h */
