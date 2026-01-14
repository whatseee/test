#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#if defined(TIME_WITH_SYS_TIME) || defined(LINUX)
#include <time.h>
#endif /* TIME_WITH_SYS_TIME */

#include "gnuc.h"
#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif /* HAVE_OS_PROTO_H */

#include "gmt2local.h"

#ifndef NO_CVS_IDENTIFY
static char CVSID[] = "$RCSfile: gmt2local.c,v $ -- $Revision: 1.7 $\n";
#endif

/*
 * Returns the difference between gmt and local time in seconds.
 * Use gmtime() and localtime() to keep things simple.
 */
int32_t
gmt2local (time_t t)
{
  register int dt, dir;
  register struct tm *gmt, *loc;
  struct tm sgmt;

  if (t == 0)
    t = time (NULL);
  gmt = &sgmt;
  *gmt = *gmtime (&t);
  loc = localtime (&t);
  dt = (loc->tm_hour - gmt->tm_hour) * 60 * 60 +
    (loc->tm_min - gmt->tm_min) * 60;

  /*
   * If the year or julian day is different, we span 00:00 GMT
   * and must add or subtract a day. Check the year first to
   * avoid problems when the julian day wraps.
   */
  dir = loc->tm_year - gmt->tm_year;
  if (dir == 0)
    dir = loc->tm_yday - gmt->tm_yday;
  dt += dir * 24 * 60 * 60;

  return (dt);
}
