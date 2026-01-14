#include "my_malloc_free.h"
#include <signal.h>

unsigned int sigprocmask_count=0;
unsigned int alarm_handler_sign=0;

const unsigned short nCRC_16 = 0x8005;
// CRC-16 = X16 + X15 + X2 + X0
const unsigned short nCRC_CCITT = 0x1021;
// CRC-CCITT = X16 + X12 + X5 + X0，据说这个 16 位 CRC 多项式比上一个要好

void *my_malloc(size_t size)
{
	void *ptr = NULL;
	ptr = malloc(size);
	
	if(!ptr)
		printf("Pay attention! malloc return NULL!\n");
	
	return ptr;
}

void my_free(void *ptr)
{
	if (ptr)
		free(ptr);
	ptr = NULL;
}

int my_sigprocmask(int how, sigset_t *set, sigset_t *oldset)
{
	if(how == SIG_BLOCK)
	{
		sigprocmask_count++;
		//printf("Gateway-->SIG_BLOCK: sigprocmask_count=%d!\n", sigprocmask_count);
		//fflush(stdout);
		if(sigprocmask_count==1)
			return my_sigprocmask (SIG_BLOCK, set, 0x0);
	}
	else if(how == SIG_UNBLOCK)
	{
		sigprocmask_count--;
		//printf("Gateway-->SIG_UNBLOCK: sigprocmask_count=%d!\n", sigprocmask_count);
		//fflush(stdout);
		if(sigprocmask_count==0)
			return my_sigprocmask (SIG_UNBLOCK, set, 0x0);
	}
	else
	{
		return my_sigprocmask (how, set, oldset);
	}

	return -1;
}

// 构造 16 位 CRC 表
void BuildTable16( unsigned short aPoly )
{
	unsigned short i, j;
	unsigned short nData;
	unsigned short nAccum;

	for ( i = 0; i < 256; i++ )
	{
		nData = ( unsigned short )( i << 8 );
		nAccum = 0;
		for ( j = 0; j < 8; j++ )
		{
			if ( ( nData ^ nAccum ) & 0x8000 )
				nAccum = ( nAccum << 1 ) ^ aPoly;
			else
				nAccum <<= 1;
			nData <<= 1;
		}
		Table_CRC[i] = ( unsigned long )nAccum;
	}
}

// 计算 16 位 CRC 值，CRC-16 或 CRC-CCITT
unsigned short CRC_16( unsigned char * aData, unsigned long aSize )
{
	unsigned long i;
	unsigned short nAccum = 0;

	for ( i = 0; i < aSize; i++ )
		nAccum = ( nAccum << 8 ) ^ ( unsigned short )Table_CRC[( nAccum >> 8 ) ^ *aData++];
	return nAccum;
}


