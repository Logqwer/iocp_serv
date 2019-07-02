#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "IOInfo.h"

class SocketInfo {
public:
	SocketInfo();
	~SocketInfo();

public:
	static SocketInfo* AllocateSocketInfo(const SOCKET& socket);
	static void DeallocateSocketInfo(SocketInfo* lpSocketInfo);

public:
	SOCKET socket;
	IOInfo* recvBuf;
	IOInfo* sendBuf;
};
