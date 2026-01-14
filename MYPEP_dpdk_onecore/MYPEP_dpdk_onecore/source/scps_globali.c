#include "../include/scps.h"
#include "scpstp.h"
#include "buffer.h"
#include "route.h"
#include "rt_alloc.h"

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: scps_globali.c,v $ -- $Revision: 1.18 $\n";
#endif

extern void *my_malloc (size_t size);
  
int tp_mssdflt = 512;
int tp_mssmin = 32;
 
char config_span_name[25] = "" "\0";
char config_local_name[25] = "" "\0";
 
int persist_time_outs [TP_MAX_PERSIST_SHIFT] = {0, 5, 6, 12, 24, 48, 60};
 
extern route *route_list_head; 
route *def_route = NULL;
route *other_route = NULL;
 
void
init_default_routes ()
 
{
	int rc; 
 
	rc = route_create_default (2000000, 1500, 1500);
	if (rc != 1) {
		printf ("Error on creating default routes I\n");
	}
	def_route = route_list_head; 

#ifdef GATEWAY
	rc = route_create_default (2000000, 1500, 1500);
	if (rc != 1) {
		printf ("Error on creating default routes II\n");
	}
	other_route = route_list_head;
#endif /* GATEWAY */
}
