#pragma once

typedef unsigned (__stdcall *PTHREAD_START) (void *);

#define BEGINTHREADEX(psa, cbStack, pfnStartAddr, \
   pvParam, fdwCreate, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                      \
         (void *)        (psa),                     \
         (unsigned)      (cbStack),                 \
         (PTHREAD_START) (pfnStartAddr),            \
         (void *)        (pvParam),                 \
         (unsigned)      (fdwCreate),               \
         (unsigned *)    (pdwThreadId)))


#define LOG(s) printf("[Log]: %s\n", s);

#define KILL_THREAD 9

#define PORT 9910
#define IP "10.10.10.10"

enum MessageType {
	ACCEPT = 0, 
	REJECT,
	REFRESH,
	CREATE,
	ENTER,
	EMPTY_ROOMLIST,
	SEEK_MYPOSITION,
	TEAM_CHANGE,
	READY_EVENT,
	LEAVE_GAMEROOM,
	START_GAME,

	DATA = 100,
	ROOMLIST,
	ROOM,
	CLIENT,
	PLAY_STATE,
	TRANSFORM,
	VECTOR_3,
	WORLD_STATE
};