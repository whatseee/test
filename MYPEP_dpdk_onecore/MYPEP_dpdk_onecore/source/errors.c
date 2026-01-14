#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include "systype.h"

#ifdef CLIENT
#ifdef SERVER
/* can't define both CLIENT and SERVER */
#endif /* SERVER */
#endif /* CLIENT */

#ifndef CLIENT
#ifndef SERVER
#define CLIENT 1		/* default is client */
#endif /* SERVER */
#endif /* CLIENT */

#ifndef NULL
#define NULL ((void *) 0)
#endif /* NULL */

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: errors.c,v $ -- $Revision: 1.14 $\n";
#endif

char *pname = NULL;

void my_perror ();

#ifdef CLIENT			/* output to stderr */


/*
 * Recoverable error.  Print a message, and retun to caller.
 *
 *  err_ret(str, arg1, arg2, ...)
 *
 * The string "str" must specify the conversion for any args.
 *
 */

/*
 * Print UNIX errno value.
 *
 */
void
my_perror ()
{
  char *sys_err_str ();

  fprintf (stderr, " %s\n", sys_err_str ());
}

#endif /*CLIENT */

#ifdef SERVER

/* We'll do this later. */

#endif /* SERVER */

#if !defined(errno)
extern int errno;
extern int sys_nerr;		/* # of error message strings in sys table */
#endif /* !defined(errno) */

/*
 * Portability Note:
 *  
 * sys_errlist is a portability thorn...
 *   
 * If you're compilation bombs due to it being undefined (or redefined)
 * in errors.c, this is the place to fix it. Older Linux systems (libc5.x)
 * will need to invert the logic of the below, to read
 * #ifdef LINUX
 *   extern const char *const sys_errlist[];
 * #endif LINUX
 *  
 * With other platforms, your milage may vary - sorry!
 *
 */

#if ( defined(__FreeBSD__) || defined(__NetBSD__) || defined(LINUX) )
#else
//extern char *sys_errlist[];	/* the system error message table */
#endif /* __FreeBSD__ || LINUX */

char *
sys_err_str (void)
{
	return (strerror (errno));
}

void
err_dump (text)
char *text;

{
	syslog (LOG_ERR, "Gateway: %s\n",text);
	abort ();
	exit (1);
}

/*
 * Return a string containing some additional information after a 
 * host name or adress lookup error - gethostbyname() or gethostbyaddr().
 */

/* int h_errno; */			/* host error number */
int h_nerr;			/* number of error message strings */
/* char 	*h_errlist[]; *//* the error message table */
/*
char *
host_err_str()
{
	static char	msgstr[200];

	if (h_errno != 0) {
 		if (h_errno > 0 && h_errno < h_nerr)
			sprintf(msgstr, "(%s)", h_errlist[h_errno]);
		else
			sprintf(msgstr, "(h_errno = %d)", h_errno);
	} else {
		msgstr[0] = '\0';
	}

	return(msgstr);
}
 Commented out for now. */
