#ifdef SCPS_RI_CONSOLE

#include "../include/scps.h"
#include "scpstp.h"
#include "../include/scpserrno.h"
#include "tp_debug.h"
#include "gateway.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define  MAX_FILTER_RULES_NUM    20

typedef struct _filter_rule
{
	int sign;

    uint32_t dst_ipaddr;
    uint32_t dst_netmask;
	unsigned short dst_lowport;
    unsigned short dst_higport;     

	uint32_t src_ipaddr;
    uint32_t src_netmask;
	unsigned short src_lowport;
    unsigned short src_higport;    
} filter_rule;

filter_rule filter_rules_array[MAX_FILTER_RULES_NUM];

void filter_rule_init();
int filter_rule_add(uint32_t dst_ipaddr, uint32_t dst_netmask, unsigned short dst_lowport, unsigned short dst_higport,     
					uint32_t src_ipaddr, uint32_t src_netmask, unsigned short src_lowport, unsigned short src_higport);
int filter_rule_del(unsigned int c);
int filter_rule_find_match_rule(uint32_t dst_ipaddr, unsigned short dst_port, uint32_t src_ipaddr, unsigned short src_port);
int check_input_filter_rule(uint32_t dst_ipaddr, uint32_t dst_netmask, unsigned short dst_lowport, unsigned short dst_higport,     
							uint32_t src_ipaddr, uint32_t src_netmask, unsigned short src_lowport, unsigned short src_higport);

#endif
