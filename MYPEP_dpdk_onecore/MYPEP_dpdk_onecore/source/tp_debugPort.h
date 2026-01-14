#ifndef tpDebugSocket_h
#define tpDebugSocket_h

#include "scpstp.h"

extern int tpDebugSocket;

typedef enum {
	debugAnswer_socket = 0xFFAA0011,
	debugAnswer_list
} debugPortInfoType;

typedef struct debug_socketListEntry {
	short	myPort;
	short	hisPort;
} debug_socketListEntry;

typedef struct debug_socketList {
	debug_socketListEntry entries[100];
} debug_socketList;

typedef struct debug_socketInfo {
	tp_Socket	theSocket;
	route		theRoute;
} debug_socketInfo;

typedef struct debugPortInfo {
	debugPortInfoType infoType;
	union {
		debug_socketInfo theSocketInfo;
		debug_socketListEntry theList[100];
	} data;
} debugPortInfo;

#endif /* tpDebugSocket_h */
