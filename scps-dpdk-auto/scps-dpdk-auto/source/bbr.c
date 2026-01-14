#include "bbr.h"

#define USEC_PER_MSEC 1000
#define min(a,b) (((a) < (b)) ? (a) : (b))
// BBR状态枚举
enum bbr_mode {
  BBR_STARTUP,   // 启动阶段
  BBR_DRAIN,     // 排空队列阶段
  BBR_PROBE_BW,  // 探测带宽阶段
  BBR_PROBE_RTT, // 探测RTT阶段
  BBR_NORMAL     // 正常阶段
};
static const int bbr_cwnd_gain  =  1;
static const int bbr_drain_gain = 1000 / 2885;
static const int bbr_high_gain  = 2885 / 1000+1;
static const uint32_t bbr_full_bw_cnt = 3;
static const uint32_t bbr_probe_rtt_mode_ms = 200;
float gf_cnwd_gain = 1.3;
int g_cnt = 0;
static float a_bbr_pacing[6] = {1.25, 0.75, 1.0, 1.0, 1.0, 1.0};
float a0 = 1.25;
float g_bw_max = 66;
float g_bw_min = 0.5;//zui xiao dai kuan
int g_keep_cnt = 0;
volatile ST_BBR *curbbr;

static uint32_t full_bw_reached(ST_BBR *bbr)
{
	return bbr->full_bw_reached;
}


#define PERIOD  (7)
float findMaxValue(ST_BBR *bbr,float value,int forse) 
{
    float *values = bbr->avalue;
    static int count = 0; 
	int i = 0;
	int j = 1;
    values[count] = value;

    if (count < PERIOD-1) 
	{
        count++; 
    } 
	else 
	{
        for (i = 0; i < PERIOD-1; i++) 
		{
            values[i] = values[i + 1];
        }
    }

    float maxValue = values[0]; 

    for (; j < count; j++) 
	{
        if (values[j] > maxValue) 
		{
            maxValue = values[j];
        }
    }

    return maxValue;
}
float findMaxValuelong(ST_BBR *bbr, float value) 
{
    float *values = bbr->alvalue;
    static int count = 0; 
	int i = 0;
	int j = 1;
    values[count] = value;

    if (count < 6) 
	{
        count++; 
    } 
	else 
	{
        for (i = 0; i < 6; i++) 
		{
            values[i] = values[i + 1];
        }
    }

    float maxValue = values[0]; 

    for (; j < count; j++) 
	{
        if (values[j] > maxValue) 
		{
            maxValue = values[j];
        }
    }

    return maxValue;
}


static int update_bw(tp_Socket *s, ST_BBR *bbr)
{
	int i = 0;
	uint64_t bw;
	uint8_t is_keep = 0;

	if(s == NULL || bbr == NULL)
		return;

	if(bbr->deliver < 0)
		return;

	if(bbr->mode == BBR_PROBE_RTT)
		return;
	if(bbr->restore)
	{
		bbr->restore++;
		if(bbr->restore >= 30)
		{
			bbr->restore = 0;
		}
		return;
	}

	// 根据当前最小RTT设置不同的参数
	if(bbr->min_rtt_us < 5000)
	{
		a0 = 1.30;
		//printf("abcd\n");
		a_bbr_pacing[1] = 1.0;
		a_bbr_pacing[0] = a0;
		g_bw_min = 0.3;
		gf_cnwd_gain = 5.5;
		//printf("aa%f\n",gf_cnwd_gain);
	}
	

	else if(bbr->min_rtt_us < 50000)
	{
		a0 = 1.25;
		//printf("abcd\n");
		a_bbr_pacing[1] = 0.8;
		a_bbr_pacing[0] = a0;
		g_bw_min = 0.3;
	}
	else if(bbr->min_rtt_us < 200000)
	{
		a0 = 1.15;
		//printf("abc\n");
		a_bbr_pacing[0] = a0;
		a_bbr_pacing[1] = 0.75;
		g_bw_min = 0.3;
	}
	else if(bbr->min_rtt_us < 600000)
	{
		//printf("c\n");
		a0 = 1.05;
		a_bbr_pacing[0] = a0;
		a_bbr_pacing[1] = 0.75;
		g_bw_min = 0.3;
	}
	else if(bbr->min_rtt_us >= 800000)
	{
		//gf_cnwd_gain = 2.5;
		//printf("a\n");
		a0 = 1.05;
		a_bbr_pacing[0] = a0;
		a_bbr_pacing[1] = 0.75;
	}


	bbr->pre_deliver = bbr->deliver;
	bbr->rtt_cnt++;

	if(bbr->all_time <= 50000000)
		bbr->all_time += bbr->interval_us;
	float fbw = (float)(bbr->deliver) / (float)(bbr->interval_us);
	
	bbr->curbw = fbw;
	//printf(" int = %u ,time = %u\n",bbr->interval_us, bbr->all_time);
	
	if(bbr->min_rtt_us < 100000)
	{
		bbr->keeptime = 6000000;
	}
	else if(bbr->min_rtt_us < 900000)
	{
		bbr->keeptime = 10000000;
	}
	else if(bbr->min_rtt_us > 900000)
	{
		bbr->keeptime = 20000000;
	}
	if(bbr->all_time >= bbr->keeptime)
	{
		if(bbr->tttt == 0)
		{
			printf("ttt = %d\n",bbr->tttt);
			bbr->tttt = 1;
			g_bw_max = 66;
			// if(s->packet_loss >= 3)
			// {
			// 	bbr->bloss = 1;
			// 	gf_cnwd_gain = 2.5;
			// } 
			if(s->packet_loss >= 13)
			{
				bbr->lossgain = 4.5;
			}
		}
		// if(fbw > bbr->long_bw * a0)
		// 	fbw = bbr->long_bw * a0;
	}
	// else if(bbr->all_time >= 20000000)
	// {
	// 	printf("20pack")
	// }
	else
	{
		g_bw_max = 0.3;

		if(bbr->min_rtt_us >= 900000)
		{
			g_bw_max = 0.3;
			gf_cnwd_gain = 1.2;
		}	
		else if(bbr->min_rtt_us >= 500000)
		{
			g_bw_max = 0.3;
			gf_cnwd_gain = 1.2;
		}
		else if(bbr->min_rtt_us >= 100000)
		{
			g_bw_max = 0.4;
			gf_cnwd_gain = 1.0;
		}
		if(bbr->min_rtt_us <= 52000)
		{
			g_bw_max  = 3.0;
			gf_cnwd_gain = 2.0;
		}
	}
	if(bbr->is_long_bw)
	{
		bbr->keep_long_fbw = findMaxValuelong(bbr,bbr->long_bw);
		bbr->is_long_bw = 0;
	}
	
	// if(bbr->all_time >= 10000000 && bbr->interval_us >= 400000 && bbr->bloss == 0)
	// {
	// 	g_cnt++;
	// 	printf("cnt = %d\n",g_cnt);
	// }
	if(bbr->all_time >= bbr->keeptime && s->packet_loss > 1 && bbr->bloss == 0)
	{
		g_cnt++;
		//printf("cnt = %d\n",g_cnt);
		s->packet_loss = 0;
	}
	if(g_cnt >= 1)
	{
		is_keep = 1;
		//g_bw_max = 18;
		//printf("ms = %u\n",bbr->interval_us);
		g_cnt = 0;
	}	
	if(is_keep && g_keep_cnt == 0 )
	{
		float tmpkeepbw = 0;
		for(i=0; i < 7; i++)
			tmpkeepbw += bbr->alvalue[i];
		tmpkeepbw = tmpkeepbw / 7.0;
		g_keep_cnt++;
		if(bbr->keep_long_fbw < 0.001)
			bbr->keep_long_fbw = bbr->long_bw;
		else
			bbr->keep_long_fbw = tmpkeepbw;//bbr->keep_long_fbw  * 0.5 + bbr->long_bw * 0.5;
		if(bbr->keep_long_fbw < g_bw_max)
			g_bw_max = bbr->keep_long_fbw;
		if(bbr->keep_long_fbw < g_bw_min)
			g_bw_max = g_bw_min;
		//printf("keepkeep1 = %f %f \n",g_bw_max ,tmpkeepbw);
		//printf("%f %f %f %f %f %f %f\n",bbr->alvalue[0],bbr->alvalue[1],bbr->alvalue[2],bbr->alvalue[3],bbr->alvalue[4],bbr->alvalue[5],bbr->alvalue[6],bbr->alvalue[7]);
		bbr->cycle_start_time = get_time_us();
		s->np_rqts.sned_start_time = 1000;
	}
	if(g_keep_cnt != 0)
		g_keep_cnt = 0;	
	if(bbr->cycle_start_time != 0 && get_time_us() - bbr->cycle_start_time >= 60000000 )  //wending shijian
	{
		g_bw_max = 66;
		s->np_rqts.sned_start_time = 0;
	}

	if(bbr->bloss == 1)
	{
		g_bw_min = bbr->g_max_bw;//bbr->g_max_long_bw * 1.6;
	}

	if(fbw < g_bw_min)
		fbw = g_bw_min;
	if(fbw > g_bw_max)
		fbw = g_bw_max;
	//printf("cnt = %d\n",g_cnt);
	if(s->np_rqts.fpacing_rate * a0 > g_bw_min && s->np_rqts.fpacing_rate * a0 < fbw )
	{
		if(bbr->bw_cnt != 1 )
		{
			fbw = s->np_rqts.fpacing_rate * 1.25;
			//printf("1ebw %f\n",fbw);
		}		
		else 
		{
			fbw = s->np_rqts.fpacing_rate * 1.3 *a0;
			//printf("2ebw %f\n",fbw);		
		}
	}
	bbr->fbw = findMaxValue(bbr,fbw,0);
	if(bbr->fbw < g_bw_min)
		bbr->fbw = g_bw_min;
	if(bbr->fbw > g_bw_max)
		bbr->fbw = g_bw_max;
	bbr->g_max_bw = max(bbr->g_max_bw, bbr->fbw);
	bbr->g_max_long_bw = max(bbr->g_max_long_bw, bbr->long_bw);
	// printf("loss = %d\n",bbr->bloss);
	printf("bbr->fbw = %f %f    %f\n",fbw, bbr->fbw,s->np_rqts.fpacing_rate);
	// printf("g_max_bw = %f\n",bbr->g_max_bw);
	// printf("g_max_long_bw = %f \n",bbr->g_max_long_bw);
	// printf("bbr->cwnd:%d\n",bbr->cwnd);
	// printf("bbr->cwndfree:%d\n",bbr->cwndFree);
	// printf("packets:%d\n",s->packet_loss);
	//printf("rtt:%d\n",s->brtt);
	
	if(bbr->all_time >= bbr->keeptime && bbr->bloss == 1 && bbr->g_max_bw * 5 > bbr->g_max_long_bw)
	{
		bbr->g_max_bw = bbr->g_max_long_bw * bbr->lossgain;
		bbr->fbw = bbr->g_max_bw;
	}
	 //bbr->fbw = 10.0;

	float tmplbw = 0;
	for(i=0; i < 7; i++)
		tmplbw += bbr->alvalue[i];
	if(tmplbw > 85.0)
		s->np_rqts.sned_start_time = 1000;
	//else
	//	s->np_rqts.sned_start_time = 0;
	//printf("tmplbw = %f\n",tmplbw);
	return 0;
}

static uint32_t target_cwnd(tp_Socket *s, ST_BBR *bbr, uint32_t bw, int gain,int acked)
{
	uint32_t cwnd;
	uint32_t fcwnd = 0;
	uint64_t w;

	if(bbr->mode == BBR_PROBE_RTT)
	{
		//printf("probe rtt123\n");
		return bbr->cwnd * 0.2;
	}

	
	fcwnd = bbr->fbw * bbr->min_rtt_us;
	if (full_bw_reached(bbr) == 1)
	{

	}
		
	else 
	{
		fcwnd = bbr->cwnd + acked;
		fcwnd *= 1.38;
		//fcwnd = fcwnd + acked;
	}
	bbr->cwnd_old = bbr->cwnd;	
	// if(action > 1.5 || action <= 0.1)
	// {
	// 	action = 1.0;
	// }
    fcwnd = 1.01 * fcwnd;
	//printf("f = %f\n",gf_cnwd_gain);
	
	bbr->cwnd = fcwnd;
	return bbr->cwnd;
}

static void check_full_bw_reached(tp_Socket *s,ST_BBR *bbr)
{
	float fbw_thresh;
	static int check_full = 0;
	check_full++;
	if (full_bw_reached(bbr)) //|| check_full < 10
		return;

	//bw_thresh = (uint64_t)bbr->full_bw * bbr_full_bw_thresh >> BBR_SCALE;
	fbw_thresh = (float)bbr->ffull_bw * (1.25);
	if(bbr->fbw >= fbw_thresh )
	{
		bbr->ffull_bw = bbr->fbw;
		bbr->full_bw_cnt = 0;
		
		return;
	}

	++bbr->full_bw_cnt;
	bbr->full_bw_reached = bbr->full_bw_cnt >= bbr_full_bw_cnt;
}

static void restore_bw(ST_BBR *bbr)
{
	bbr->restore = 1;
}

void update_min_rtt(tp_Socket *s, ST_BBR *bbr)
{	
	bool bprobe_rtt;
	// if(gw_ifs.probertt == 1)
	// 	bprobe_rtt = get_time_us() >= bbr->min_rtt_stamp + 12.5 * USEC_PER_SEC;
	// else
		bprobe_rtt = false;
	if (s->brtt > 0 && (s->brtt < bbr->min_rtt_us || bprobe_rtt))
	{
		//log4c(__FILE__,__FUNCTION__,"min_rtt_us = %u\n",bbr->min_rtt_us);
		bbr->min_rtt_us = s->brtt;
		bbr->min_rtt_stamp = get_time_us();
		//printf("min_rtt = %u\n",bbr->min_rtt_us);
	}

	if(bbr_probe_rtt_mode_ms > 0 && bprobe_rtt && bbr->mode != BBR_PROBE_RTT)
	{
		//log4c(__FILE__,__FUNCTION__,"probe rtt\n");
		//nowprintf("probe rtt start\n");
		//log4c(__FILE__,__FUNCTION__,"time = %u\n",get_time_us());	
		bbr->mode = BBR_PROBE_RTT;
		bbr->pacing_gain = 1;
		bbr->probe_rtt_done_stamp = 0;
	}

	if(bbr->mode == BBR_PROBE_RTT)
	{
		//log4c(__FILE__,__FUNCTION__,"s->cwnd = %d\n",bbr->cwnd);
		if(!bbr->probe_rtt_done_stamp)
		{
			bbr->probe_rtt_done_stamp = get_time_us() + bbr_probe_rtt_mode_ms * USEC_PER_MSEC;
			bbr->probe_rtt_round_done = 0;
		}
		else if(bbr->probe_rtt_done_stamp)
		{
			if(get_time_us() >= bbr->probe_rtt_done_stamp)
			{
				bbr->min_rtt_stamp = get_time_us();
				bbr->mode = BBR_NORMAL;
				restore_bw(bbr);
				//log4c(__FILE__,__FUNCTION__,"probe rtt done\n");
				//nowprintf("probe rtt done\n");
			}
		}
	}
}

static void check_drain(tp_Socket *s, ST_BBR *bbr)
{

	if (bbr->mode == BBR_STARTUP && full_bw_reached(bbr)) 
	{
		bbr->mode = BBR_DRAIN;	/* drain queue we created */
		bbr->cwnd_gain = bbr_high_gain;	/* maintain cwnd */
		bbr->cwnd_gain = bbr_drain_gain;
	}	
	
	if (bbr->mode == BBR_DRAIN && bbr->cwndUsed <= bbr->cwnd)
	{
		bbr->cwnd_gain = bbr_cwnd_gain;
		bbr->pacing_gain = 1;
		bbr->mode = BBR_NORMAL;
		bbr->min_rtt_stamp = get_time_us();
		//InterInfoInit(bbr, &(bbr->IntervalInfo));
	}
}

static void init_pacing_rate_from_rtt(tp_Socket *s, ST_BBR *bbr,int gain)
{
	uint64_t bw;
	uint32_t rtt_us;
	
	float l_gain ;
	if (s->pacing_rate) 
	{	/* any RTT sample yet? */
		rtt_us = max(bbr->srtt, 1U);
		bbr->has_seen_rtt = 1;
	} 
	else 
	{
		rtt_us = USEC_PER_MSEC;	
	}
	

	if(full_bw_reached(bbr))
	{
		l_gain = a_bbr_pacing[bbr->bw_cnt];
	}
	else
	{
		l_gain = gain;
	}
	if(bbr->mode == BBR_PROBE_RTT)
	{
		l_gain = 1.0;
	}
	s->np_rqts.fpacing_rate = bbr->fbw * l_gain;	
	//s->np_rqts.fpacing_rate = (bbr->cwnd*1.0) /(bbr->min_rtt_us*1.0) * l_gain;	
}

static void set_pacing_rate(tp_Socket *s, ST_BBR *bbr, uint32_t bw, int gain)
{
	init_pacing_rate_from_rtt(s, bbr, gain);
}

static void check_cycle(tp_Socket *s, ST_BBR *bbr)
{
	bbr->bw_cnt++;
	if(bbr->bw_cnt == 6)
	{
		bbr->bw_cnt = 0;
	}
	if(1)
	{
		bbr->cwnd_gain = bbr_high_gain;
		bbr->mode = BBR_PROBE_BW;
	}
}

static void update_model(tp_Socket *s, ST_BBR *bbr)
{
	update_bw(s,bbr);
	check_full_bw_reached(s,bbr);
	check_drain(s,bbr);
	update_min_rtt(s,bbr);
	check_cycle(s,bbr);
}

uint32_t bbr_main(tp_Socket *s, ST_BBR *bbr,int acked,uint8_t flag)
{
	uint32_t bw, cwnd;
	update_model(s, bbr);
	set_pacing_rate(s,bbr,bw,bbr->pacing_gain);


	cwnd = target_cwnd(s,bbr, bw, bbr->cwnd_gain,acked);

	//cwnd = min(cwnd, 23750000);

	return cwnd;
}

int bbr_init(tp_Socket *s)
{
    int i = 0;
	ST_BBR *bbr;
	bbr = malloc(sizeof(ST_BBR));
	if(bbr == NULL)
		return -1;
	bbr->rtt = 0;
	bbr->deliver = 0;
	bbr->rtt_cnt = 0;
	bbr->full_bw =0;
	bbr->full_bw_cnt = 0;
	bbr->full_bw_reached = 0;
	bbr->round_start = 0;
	bbr->min_rtt_us = 1500000;
	bbr->min_rtt_stamp = 0;
	bbr->cwnd = 4;
	bbr->is_ack = 1;
	bbr->mode = BBR_STARTUP;
	bbr->cwnd_gain = bbr_high_gain;
	
	bbr->min_rtt_stamp = get_time_us();
	bbr->pacing_gain = bbr_high_gain;
	bbr->srtt = 0;
	bbr->has_seen_rtt = 0;
	bbr->drtt = 0;
	bbr->cwndUsed = 0;
	bbr->interval_us = 0;
	bbr->cwndFree = 0;
	bbr->curbw = 0;
	bbr->hole = 0;
	bbr-> is_long_bw;
	bbr->all_time = 0;
	bbr->cycle_start_time = 0;
	bbr->fbw = 0;
	bbr->g_max_bw = 0;
	bbr->g_max_long_bw = 0;
	bbr->bloss = 0;
	bbr->lossgain = 2.25;
	bbr->keeptime = 0;
	bbr->keep_long_fbw = 0;
	for( i = 0; i < 17; i++)
	{
		bbr->avalue[i] = 0;
		bbr->alvalue[i] = 0;
	}

	s->bbr = bbr;
	g_bw_max = 70;
	g_bw_min = 0.3;
	g_keep_cnt = 0;
	bbr->tttt = 0;
	return 0;
}
uint32_t get_time_us(void)
{
  uint32_t timenow;
  struct timeval mytime;

  gettimeofday (&mytime, NULL);

  timenow = (mytime.tv_sec * 1000000) + (mytime.tv_usec);

  return (timenow);
}