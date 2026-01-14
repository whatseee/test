/*
 * scps_config.c - configuration parameters for scps_initiator
 *                 and scps_responder
 */

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: scps_config.c,v $ -- $Revision: 1.6 $\n";
#endif

char config_init_name[25] = "vigo" "\0";
char config_resp_name[25] = "ratbert" "\0";

int config_tp_pkt_count = 10000;	/* set to 0 to disable TP */
int config_tp_pkt_size = 1000;
int config_tp_read_size = 10000;
int config_udp_pkt_count = 0;	/* set to 0 to disable UDP */
int config_udp_pkt_size = 100;
int config_udp_read_size = 500;

unsigned short config_tp_init_port = 0x8000;
unsigned short config_tp_resp_port = 0x8fff;
unsigned short config_udp_init_port = 0x0555;
unsigned short config_udp_resp_port = 0x0554;
