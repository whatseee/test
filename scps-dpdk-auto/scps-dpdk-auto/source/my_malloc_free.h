#include <stdlib.h>
#include <stdio.h>

extern void *my_malloc(size_t size);
extern void my_free(void *ptr);
int my_sigprocmask(int how, sigset_t *set, sigset_t *oldset);
extern void BuildTable16( unsigned short aPoly );
extern unsigned short CRC_16( unsigned char * aData, unsigned long aSize );


extern unsigned int sigprocmask_count;
extern unsigned int alarm_handler_sign;
extern const unsigned short nCRC_16;
extern const unsigned short nCRC_CCITT;
extern unsigned long Table_CRC[256]; // CRC 表
