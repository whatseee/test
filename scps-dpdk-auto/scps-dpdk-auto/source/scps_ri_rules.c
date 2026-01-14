#ifdef SCPS_RI_CONSOLE

#include "scps_ri_rules.h"
#include "scps_ri_console.h"

void filter_rule_init()
{
	int i = 0;
	
	for(i=0; i<MAX_FILTER_RULES_NUM; i++)
	{
		memset(&filter_rules_array[i], 0, sizeof(filter_rule));
	}
}

int filter_rule_add(uint32_t dst_ipaddr, uint32_t dst_netmask, unsigned short dst_lowport, unsigned short dst_higport,     
					uint32_t src_ipaddr, uint32_t src_netmask, unsigned short src_lowport, unsigned short src_higport)
{
	int i = 0;
	
	for(i=0; i<MAX_FILTER_RULES_NUM; i++)
	{
		if(filter_rules_array[i].sign == 0)
		{
			filter_rules_array[i].sign = 1;

			filter_rules_array[i].dst_ipaddr  = dst_ipaddr;
			filter_rules_array[i].dst_netmask = dst_netmask;
			filter_rules_array[i].dst_lowport = dst_lowport;
			filter_rules_array[i].dst_higport = dst_higport;     

			filter_rules_array[i].src_ipaddr  = src_ipaddr;
			filter_rules_array[i].src_netmask = src_netmask;
			filter_rules_array[i].src_lowport = src_lowport;
			filter_rules_array[i].src_higport = src_higport; 

			break;
		}
	}

	if(i >= MAX_FILTER_RULES_NUM)
		return -1;
	else
		return i;
}

int filter_rule_del(unsigned int c)
{
	if(c >= MAX_FILTER_RULES_NUM) 
		return -1;

	memset(&filter_rules_array[c], 0, sizeof(filter_rule));

	return c;
}

int filter_rule_find_match_rule(uint32_t dst_ipaddr, unsigned short dst_port, uint32_t src_ipaddr, unsigned short src_port)
{
	int i = 0;
	
	for(i=0; i<MAX_FILTER_RULES_NUM; i++)
	{
		if(filter_rules_array[i].sign == 1)
		{
			if( ((dst_ipaddr&filter_rules_array[i].dst_netmask)==(filter_rules_array[i].dst_ipaddr&filter_rules_array[i].dst_netmask)) &&				
				((src_ipaddr&filter_rules_array[i].src_netmask)==(filter_rules_array[i].src_ipaddr&filter_rules_array[i].src_netmask))
			  )
			{
				if( ((filter_rules_array[i].dst_lowport<=dst_port)&&(dst_port<=filter_rules_array[i].dst_higport)) &&
				    ((filter_rules_array[i].src_lowport<=src_port)&&(src_port<=filter_rules_array[i].src_higport))
				  )
				{
					break;
				}
			}

			if( ((dst_ipaddr&filter_rules_array[i].src_netmask)==(filter_rules_array[i].src_ipaddr&filter_rules_array[i].src_netmask)) &&				
				((src_ipaddr&filter_rules_array[i].dst_netmask)==(filter_rules_array[i].dst_ipaddr&filter_rules_array[i].dst_netmask))
			  )
			{
				if( ((filter_rules_array[i].src_lowport<=dst_port)&&(dst_port<=filter_rules_array[i].src_higport)) &&
				    ((filter_rules_array[i].dst_lowport<=src_port)&&(src_port<=filter_rules_array[i].dst_higport))
				  )
				{
					break;
				}
			}
		}
	}

	if(i >= MAX_FILTER_RULES_NUM)
		return -1;
	else
		return i;
}

int check_input_filter_rule(uint32_t dst_ipaddr, uint32_t dst_netmask, unsigned short dst_lowport, unsigned short dst_higport,     
							uint32_t src_ipaddr, uint32_t src_netmask, unsigned short src_lowport, unsigned short src_higport)
{
	int i = 0;
	int r1 = 0;
	int r2 = 0;
	int r3 = 0;
	int r4 = 0;

	for(i=0; i<MAX_FILTER_RULES_NUM; i++)
	{
		if(filter_rules_array[i].sign == 1)
		{
			r1 = are_two_nets_overlapped(dst_ipaddr, dst_netmask, filter_rules_array[i].dst_ipaddr, filter_rules_array[i].dst_netmask);
			r2 = are_two_nets_overlapped(src_ipaddr, src_netmask, filter_rules_array[i].src_ipaddr, filter_rules_array[i].src_netmask);
			r3 = are_two_sport_groups_overlapped(dst_lowport, dst_higport, filter_rules_array[i].dst_lowport, filter_rules_array[i].dst_higport);
			r4 = are_two_sport_groups_overlapped(src_lowport, src_higport, filter_rules_array[i].src_lowport, filter_rules_array[i].src_higport);
		
			if( (r1==1) && (r2==1) && (r3==1) && (r4==1) )  //不能添加
				return 1;
		}
	}

	return 0;			//可以添加
}
#endif