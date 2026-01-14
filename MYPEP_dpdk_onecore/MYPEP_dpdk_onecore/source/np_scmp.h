/*
 * Definition of type and code field values
 */
#define SCMP_ECHOREPLY		0	/* echo reply */
#define SCMP_UNREACH            3	/* dest unreachable; codes: */
#define		SCMP_UNREACH_NET	0	/* bad net */
#define		SCMP_UNREACH_HOST	1	/* bad host */
#define		SCMP_UNREACH_PROTOCOL	2	/* bad protocol */
#define		SCMP_UNREACH_PORT	3	/* bad port */
#define		SCMP_UNREACH_NEEDFRAG	4	/* datagram too large */
#define		SCMP_UNREACH_UNHOST	7	/* unknown dest host */
#define		SCMP_UNREACH_QOSNET	11	/* for QoS and net */
#define		SCMP_UNREACH_QOSHOST	12	/* for QoS and host */
#define		SCMP_UNREACH_ADMIN	13	/* for admin filter */
#define		SCMP_UNREACH_HOSTPREC	14	/* for host precedence */
#define		SCMP_UNREACH_MINPREC	15	/* for too-low prec */
#define		SCMP_UNREACH_LINKOUT	16	/* for link outage */
#define SCMP_SOURCEQUENCH	4	/* packet lost, slow down */
#define SCMP_REDIRECT		5	/* shorter route; codes: */
#define		SCMP_REDIRECT_HOST	1	/* for host */
#define 	SCMP_REDIRECT_QOSHOST	3	/* for QoS and host */
#define 	SCMP_REDIRECT_LINK	4	/* link now available */
#define SCMP_ECHO		8	/* echo service */
#define SCMP_TIMXCEED		11	/* time exceeded */
#define SCMP_PARAMPROB		12	/* np header bad */
#define SCMP_CORRUPT		19	/* corruption experienced */
#define NONE                    -1	/* no code defined */
