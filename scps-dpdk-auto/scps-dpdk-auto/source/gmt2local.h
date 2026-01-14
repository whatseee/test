#ifndef gmt2local_h
#define gmt2local_h

#if defined(Sparc)
int gmt2local (time_t);
#else /* defined(Sparc) */
int32_t gmt2local (time_t);
#endif /* defined(Sparc) */
#endif /* gmt2local_h */
