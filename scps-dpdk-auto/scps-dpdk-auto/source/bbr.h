#ifndef BBR_H__
#define BBR_H__
#include "scpstp.h"

typedef struct 
{
	uint32_t rtt;
	uint32_t deliver;
	uint32_t pre_deliver;
	//struct minmax bw;
	uint32_t full_bw;
	uint32_t full_bw_cnt;
	uint32_t full_bw_reached;
	int cwnd;
	int cwnd_old;
	uint32_t rtt_cnt;
	uint16_t round_start;
	uint32_t min_rtt_us;
	uint32_t min_rtt_stamp;
	uint16_t is_ack;
	uint32_t cwnd_gain;
	uint32_t mode;
	uint32_t srtt;
	float curbw;
	uint16_t has_seen_rtt;
	uint16_t pacing_gain;
	uint32_t drtt;
	uint32_t min_rtt_down_stamp;
	uint32_t interval_us;
	uint32_t pre_time;
	// StIntervalInfo IntervalInfo;
	// ST_KEEP_CP st_keep_cp;
	int cwndUsed;
	int cwndFree;
	float fbw;
	float ffull_bw;
	uint32_t hole;
	uint8_t bw_cnt;
	uint32_t min_rtt_10s;
	uint32_t min_rtt_10s_down;
	uint32_t probe_rtt_done_stamp;
	uint32_t probe_rtt_round_done;
	uint32_t restore;  
	float long_bw;
	uint32_t is_long_bw;
    uint32_t all_time;
    uint32_t cycle_start_time;
    float g_max_bw;
    float g_max_long_bw;
    int bloss;
    int tttt ;
    float lossgain;
    uint32_t keeptime;
    float avalue[18];
    float alvalue[18];
    float keep_long_fbw;


}ST_BBR;

int bbr_init(tp_Socket *s);
uint32_t get_cwnd(tp_Socket *s, ST_BBR *bbr, int acked);
uint32_t bbr_main(tp_Socket *s, ST_BBR *bbr,int acked,uint8_t flag);
uint32_t get_time_us(void);
void update_min_rtt(tp_Socket *s, ST_BBR *bbr);
#endif
