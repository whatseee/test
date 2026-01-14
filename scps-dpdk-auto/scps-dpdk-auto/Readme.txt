There are 4 places modified in tp_handler.c:
[1] in line 1692, cancel send SYN+ACK in StateLISTEN 
#if 0  //for vipersat
				if ((mbuffer = tp_BuildHdr (new_socket, NULL, 0)))
					enq_mbuff (mbuffer, new_socket->send_buff);

				if (!(new_socket->send_buff->send))
					new_socket->send_buff->send = new_socket->send_buff->last;

				if (new_socket->send_buff->send)
				{
					tp_NewSend (new_socket, NULL, false);
				}

				new_socket->state_prev = new_socket->state;
				new_socket->state = tp_StateSYNREC;
				PRINT_STATE (s->state, s);
#endif

[2] in line 1753 cancel set KeepTimer in StateLISTEN
#if 0		//for vipersat
				mytime_keep.tv_sec = 60;
				mytime_keep.tv_usec = 0;
				set_timer(&mytime_keep, new_socket->otimers[Keep], 1);
#endif

[3] in line 1944 get MAC address in StateSYNSENT
/****************************************** for vipersat *******************************************/
#ifdef TAP_INTERFACE
		memcpy (&(s->peer_socket->src_mac_addr [0]), &(rqts->src_mac_addr [0]),6);
		memcpy (&(s->peer_socket->dst_mac_addr [0]), &(rqts->dst_mac_addr [0]),6);
#endif /* TAP_INTERFACE */
/****************************************** for vipersat *******************************************/

[4] in line 1958 sent SYN+ACK in StateSYNSENT
/****************************************** for vipersat *******************************************/
				if ((mbuffer = tp_BuildHdr (s->peer_socket, NULL, 0)))
					enq_mbuff (mbuffer, s->peer_socket->send_buff);

				if (!(s->peer_socket->send_buff->send))
					s->peer_socket->send_buff->send = s->peer_socket->send_buff->last;

				if (s->peer_socket->send_buff->send)
				{
					tp_NewSend (s->peer_socket, NULL, false);
				}

				s->peer_socket->state_prev = s->peer_socket->state;
				s->peer_socket->state = tp_StateSYNREC;
				PRINT_STATE (s->state, s);

				mytime_keep.tv_sec = 60;
				mytime_keep.tv_usec = 0;
				set_timer(&mytime_keep, s->peer_socket->otimers[Keep], 1);
/****************************************** for vipersat *******************************************/


