#ifdef SCPS_RI_CONSOLE
#ifndef SCPS_RI_CONSOLE_H
#define SCPS_RI_CONSOLE_H

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "../include/scps.h"
#include "route.h"
#include "rt_alloc.h"
#include "net_types.h"

void parse_console_command (unsigned char *data, struct sockaddr_in *sin, int len);
void console_route_timer (gateway_command_t *cmd, struct sockaddr_in *sin_to, int sin_to_len);
void console_route_add (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void console_route_delete (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void console_route_list (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void console_route_get (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void console_route_modify (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void console_route_avail (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void console_route_unavail (gateway_command_t *cmd, struct sockaddr_in *sin, int len);
void read_scps_ri_console (void);

//////////////////////////////////////////////////////
unsigned int my_power(unsigned int d, int power);
int are_two_nets_overlapped(unsigned int ip1, unsigned int mask1, unsigned int ip2, unsigned int mask2);
int are_two_nets_overlapped_2(unsigned int ip1, unsigned int mask1, unsigned int ip2, unsigned int mask2);
int are_two_sport_groups_overlapped(unsigned short port_start1, unsigned short port_end1, unsigned short port_start2, unsigned short port_end2);
int check_input_route(uint32_t snet, uint32_t smask, uint32_t dnet, uint32_t dmask);

void console_filter_rule_add(gateway_command_t *cmd, struct sockaddr_in *sin_to, int sin_to_len);
void console_filter_rule_delete(gateway_command_t *cmd, struct sockaddr_in *sin_to, int	sin_to_len);
void console_filter_rule_list(gateway_command_t *cmd, struct sockaddr_in *sin_to, int	sin_to_len);

#endif /* SCPS_RI_CONSOLE_H */
#endif /* SCPS_RI_CONSOLE */
