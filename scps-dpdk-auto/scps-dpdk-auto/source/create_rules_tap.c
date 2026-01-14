#ifdef GATEWAY
#ifdef TAP_INTERFACE

#include <stdarg.h>
#include "rs_config.h"
#include "ll.h"
#include "route.h"
#include "tap.h"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif /* min */

extern int scps_udp_port;
extern int scps_udp_port1;
extern int nl_default;

#ifdef GATEWAY_DUAL_INTERFACE
extern int special_port_number;
#endif /* GATEWAY_DUAL_INTERFACE */

#ifdef EXTERNAL_RULE_GENERATION
#define SYSTEM(A)
#else /* EXTERNAL_RULE_GENERATION */
#define SYSTEM(A) system(A)
#endif /* EXTERNAL_RULE_GENERATION */

extern GW_ifs gw_ifs;

static char aif_addr_string[256];
static char aif_netmask_string[256];
static char bif_addr_string[256];
static char bif_netmask_string[256];

#define ORIG_TABLES "/tmp/saved.tbl"
void gateway_tap_cleanup(int a)
{
  	char iptables_cmd[256];
    sigset_t sigset;
    sigset_t oldset;
    sigfillset(&sigset);
    //sigprocmask(SIG_BLOCK,&sigset,&oldset);

#ifdef LINUX
printf("hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\n");
fflush(stdout);

  sprintf (iptables_cmd, "ifconfig aif down");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "brctl delif aif %s", gw_ifs.aif_tap_name);
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if (!(gw_ifs.aif_tap_no_phy)) {
  	sprintf (iptables_cmd, "brctl delif aif %s", gw_ifs.aif_name);
  	printf ("iptables:: %s\n", iptables_cmd);
	SYSTEM (iptables_cmd);
  }

  sprintf (iptables_cmd, "brctl delbr aif");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig bif down");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "brctl delif bif %s", gw_ifs.bif_tap_name);
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if (!(gw_ifs.bif_tap_no_phy)) {
  	sprintf (iptables_cmd, "brctl delif bif %s", gw_ifs.bif_name);
 	printf ("iptables:: %s\n", iptables_cmd);
 	SYSTEM (iptables_cmd);
  }

  sprintf (iptables_cmd, "brctl delbr bif");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if ((gw_ifs.c_tap_remote_access == 1) && (a == 1))  {
    if (!(gw_ifs.aif_tap_no_phy)) {
    	sprintf (iptables_cmd, "ifconfig %s %s netmask %s", gw_ifs.aif_name, aif_addr_string, aif_netmask_string);
    	printf ("iptables:: %s\n", iptables_cmd);
    	SYSTEM (iptables_cmd);
    }
/*
    if (!(gw_ifs.bif_tap_no_phy)) {
    	sprintf (iptables_cmd, "ifconfig %s %s netmask %s", gw_ifs.bif_name, bif_addr_string, bif_netmask_string);
    	printf ("iptables:: %s\n", iptables_cmd);
    	SYSTEM (iptables_cmd);
    }
*/
  }
    //sigprocmask(SIG_SETMASK,&oldset,0);
#endif /* LINUX */

/* 
 * FreeBSD and NetBSD use the same if_bridge(4) code 
*/
#if defined( __FreeBSD__) || defined(__NetBSD__)

  sprintf (iptables_cmd, "ifconfig bridge0 destroy");
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig bridge1 destroy");
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if ((gw_ifs.c_tap_remote_access == 1) && (a == 1))  {
    if (!(gw_ifs.aif_tap_no_phy)) {
    	sprintf (iptables_cmd, "ifconfig %s inet %s netmask %s", gw_ifs.aif_name, aif_addr_string, aif_netmask_string);
    	printf ("ifconfig:: %s\n", iptables_cmd);
    	SYSTEM (iptables_cmd);
    
    	if (gw_ifs.aif_mtu) {
        	sprintf (iptables_cmd, "ifconfig %s mtu %i", gw_ifs.aif_name, gw_ifs.aif_mtu); 
        	printf ("ifconfig:: %s\n", iptables_cmd);
      	   	SYSTEM (iptables_cmd);
   	}
    }
  }

    if (!(gw_ifs.bif_tap_no_phy)) {
    	sprintf (iptables_cmd, "ifconfig %s inet %s netmask %s", gw_ifs.bif_name, bif_addr_string, bif_netmask_string);
    	printf ("ifconfig:: %s\n", iptables_cmd);
    	SYSTEM (iptables_cmd);

    	if (gw_ifs.bif_mtu) {
      	  	sprintf (iptables_cmd, "ifconfig %s mtu %i", gw_ifs.bif_name, gw_ifs.bif_mtu); 
         	printf ("ifconfig:: %s\n", iptables_cmd);
         	SYSTEM (iptables_cmd);
    	}

    }
    //sigprocmask(SIG_SETMASK,&oldset,0);
#endif /* __FreeBSD__ || __NetBSD__ */

}


void
gateway_tap_rules ()

{
  int temp_sd;
#ifdef MPF
  int j;
#endif /* MPF */
  int retval = 0;
  struct ifreq if_str;
  struct sockaddr_in saddr;

  char iptables_cmd[256];

  gateway_tap_cleanup (0);


  if (gw_ifs.c_tap_remote_access == 1) {
  if (!(gw_ifs.aif_tap_no_phy)) {
    /* get the local addrs for both A and B interfaces */
    temp_sd = socket (PF_INET, SOCK_DGRAM, 0);

    memset (&if_str, 0, sizeof (struct ifreq));
    strcpy (if_str.ifr_name, gw_ifs.aif_name);

    retval = ioctl (temp_sd, SIOCGIFADDR, &if_str);
    if (retval != 0) {
      printf ("FATAL ERROR: ioctl returned %d for specified AIF_NAME %s. \n",
	      retval, gw_ifs.aif_name);
      printf ("Re-enter AIF_NAME into resource file.\n");
      printf ("Choose valid AIF_NAME from among the following: \n");
      system ("netstat -i");
      printf ("\n");
      printf ("GATEWAY ABORTING........\n\n");
      exit (-1);
    }
    memcpy (&(saddr), &(if_str.ifr_addr), sizeof (struct sockaddr));
    strcpy (aif_addr_string, inet_ntoa (saddr.sin_addr));

    memset (&if_str, 0, sizeof (struct ifreq));
    strcpy (if_str.ifr_name, gw_ifs.aif_name);

    retval = ioctl (temp_sd, SIOCGIFNETMASK, &if_str);

    if (retval != 0) {
      printf ("FATAL ERROR: ioctl returned %d for specified AIF_NAME %s. \n",
	      retval, gw_ifs.aif_name);
      printf ("Re-enter AIF_NAME into resource file.\n");
      printf ("Choose valid AIF_NAME from among the following: \n");
      system ("netstat -i");
      printf ("\n");
      printf ("GATEWAY ABORTING........\n\n");
      exit (-1);
    }

    memcpy (&(saddr), &(if_str.ifr_addr), sizeof (struct sockaddr));
    strcpy (aif_netmask_string, inet_ntoa (saddr.sin_addr));
    }
/*
  if (!(gw_ifs.bif_tap_no_phy)) {
    memset (&if_str, 0, sizeof (struct ifreq));
    strcpy (if_str.ifr_name, gw_ifs.bif_name);

  retval = ioctl (temp_sd, SIOCGIFADDR, &if_str);
    if (retval != 0) {
      printf ("FATAL ERROR: ioctl returned %d for specified AIF_NAME %s. \n",
	      retval, gw_ifs.bif_name);
      printf ("Re-enter AIF_NAME into resource file.\n");
      printf ("Choose valid AIF_NAME from among the following: \n");
      system ("netstat -i");
      printf ("\n");
      printf ("GATEWAY ABORTING........\n\n");
      exit (-1);
    }
    memcpy (&(saddr), &(if_str.ifr_addr), sizeof (struct sockaddr));
    strcpy (bif_addr_string, inet_ntoa (saddr.sin_addr));

    memset (&if_str, 0, sizeof (struct ifreq));
    strcpy (if_str.ifr_name, gw_ifs.bif_name);

    retval = ioctl (temp_sd, SIOCGIFNETMASK, &if_str);

    if (retval != 0) {
      printf ("FATAL ERROR: ioctl returned %d for specified AIF_NAME %s. \n",
	      retval, gw_ifs.bif_name);
      printf ("Re-enter AIF_NAME into resource file.\n");
      printf ("Choose valid AIF_NAME from among the following: \n");
      system ("netstat -i");
      printf ("\n");
      printf ("GATEWAY ABORTING........\n\n");
      exit (-1);
    }
    memcpy (&(saddr), &(if_str.ifr_addr), sizeof (struct sockaddr));
    strcpy (bif_netmask_string, inet_ntoa (saddr.sin_addr));

    printf ("Got %s interface address:  %s %s\n", gw_ifs.bif_name, bif_addr_string, bif_netmask_string);
    }
*/
    }
#ifdef LINUX
/*
  sprintf (iptables_cmd, "ip addr add 10.99.99.1 peer 10.99.99.2 dev %s", gw_ifs.aif_tap_name);
  printf("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);
  sprintf (iptables_cmd, "ip addr add 10.99.98.1 peer 10.99.98.2 dev %s" ,gw_ifs.bif_tap_name);
  printf("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);
*/

  sprintf (iptables_cmd, "ip link set dev %s up", gw_ifs.aif_tap_name);
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ip link set dev %s up", gw_ifs.bif_tap_name);
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "brctl addbr aif");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "brctl addif aif %s", gw_ifs.aif_tap_name);
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if (!(gw_ifs.aif_tap_no_phy)) {
  	sprintf (iptables_cmd, "brctl addif aif %s", gw_ifs.aif_name);
  	printf ("iptables:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  }

  sprintf (iptables_cmd, "brctl stp aif off");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig aif up");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "brctl addbr bif");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "brctl addif bif %s", gw_ifs.bif_tap_name);
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if (!(gw_ifs.bif_tap_no_phy)) {
  	sprintf (iptables_cmd, "brctl addif bif %s", gw_ifs.bif_name);
 	 printf ("iptables:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  }

  sprintf (iptables_cmd, "brctl stp bif off");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig bif up");
  printf ("iptables:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if (gw_ifs.c_tap_remote_access == 1) {
  if (!(gw_ifs.aif_tap_no_phy)) {
    sprintf (iptables_cmd, "ip addr del %s dev %s", aif_addr_string, gw_ifs.aif_name);
    printf ("iptables:: %s\n", iptables_cmd);
    SYSTEM (iptables_cmd);

    sprintf (iptables_cmd, "ifconfig %s %s netmask %s", "aif", aif_addr_string, aif_netmask_string);
    printf ("iptables:: %s\n", iptables_cmd);
    SYSTEM (iptables_cmd);
  }
/*
  if (!(gw_ifs.aif_tap_no_phy)) {
    sprintf (iptables_cmd, "ip addr del %s dev %s", bif_addr_string, gw_ifs.bif_name);
    printf ("iptables:: %s\n", iptables_cmd);
    SYSTEM (iptables_cmd);

    sprintf (iptables_cmd, "ifconfig %s %s netmask %s", "bif", bif_addr_string, bif_netmask_string);
    printf ("iptables:: %s\n", iptables_cmd);
    SYSTEM (iptables_cmd);
  }
*/
  }
#endif /* LINUX */

#if defined(__FreeBSD__) || defined(__NetBSD__)
  sprintf (iptables_cmd, "ifconfig bridge0 create");
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig bridge1 create");
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);
#endif /* __FreeBSD__ || __NetBSD__ */

#ifdef __FreeBSD__
  sprintf (iptables_cmd, "ifconfig %s up", gw_ifs.bif_tap_name);
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig %s up", gw_ifs.aif_tap_name);
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  if (!(gw_ifs.aif_tap_no_phy)) {
  	sprintf (iptables_cmd, "ifconfig bridge0 addm %s addm %s up", gw_ifs.aif_name, gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "ifconfig bridge0 -stp %s -stp %s", gw_ifs.aif_name, gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  } else {
  	sprintf (iptables_cmd, "ifconfig bridge0 addm %s up", gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "ifconfig bridge0 -stp %s", gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  }

  if (!(gw_ifs.aif_tap_no_phy)) {
  	sprintf (iptables_cmd, "ifconfig bridge1 addm %s addm %s up", gw_ifs.bif_name, gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "ifconfig bridge1 -stp %s -stp %s", gw_ifs.bif_name, gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  } else {
  	sprintf (iptables_cmd, "ifconfig bridge1 addm %s up", gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "ifconfig bridge1 -stp %s", gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  }

#endif /* __FreeBSD __ */

#ifdef __NetBSD__
  if (!(gw_ifs.aif_tap_no_phy)) {
  	sprintf (iptables_cmd, "brconfig bridge0 add %s add %s up", gw_ifs.aif_name, gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "brconfig bridge0 -stp %s -stp %s", gw_ifs.aif_name, gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  } else {
  	sprintf (iptables_cmd, "brconfig bridge0 add %s up", gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "brconfig bridge0 -stp %s", gw_ifs.aif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  }

  if (!(gw_ifs.bif_tap_no_phy)) {
  	sprintf (iptables_cmd, "brconfig bridge1 add %s add %s up", gw_ifs.bif_name, gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "brconfig bridge1 -stp %s -stp %s", gw_ifs.bif_name, gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);
  } else {
  	sprintf (iptables_cmd, "brconfig bridge1 add %s up", gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  	sprintf (iptables_cmd, "brconfig bridge1 -stp %s", gw_ifs.bif_tap_name);
  	printf ("ifconfig:: %s\n", iptables_cmd);
  	SYSTEM (iptables_cmd);

  }

  sprintf (iptables_cmd, "ifconfig %s up", gw_ifs.bif_tap_name);
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);

  sprintf (iptables_cmd, "ifconfig %s up", gw_ifs.aif_tap_name);
  printf ("ifconfig:: %s\n", iptables_cmd);
  SYSTEM (iptables_cmd);
#endif /* __NetBSD__ */

}

#endif /* TAP_INTERFACE */
#endif /* GATEWAY */
