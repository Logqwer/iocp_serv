#include "SocketInfo.h"
#include "ErrorHandle.h"
#include <cassert>

SocketInfo::SocketInfo() 
{
	socket = INVALID_SOCKET;
	recvBuf = NULL;
	sendBuf = NULL;
}

SocketInfo::~SocketInfo() 
{
}

SocketInfo* SocketInfo::AllocateSocketInfo(const SOCKET& socket)
{
	SocketInfo* lpSocketInfo = new SocketInfo();
	assert(lpSocketInfo != NULL);
	lpSocketInfo->socket = socket;
	lpSocketInfo->recvBuf = IOInfo::AllocateIoInfo();
	lpSocketInfo->sendBuf = IOInfo::AllocateIoInfo();

	return lpSocketInfo;
}

void SocketInfo::DeallocateSocketInfo(SocketInfo* lpSocketInfo)
{
	assert(lpSocketInfo != NULL);
	if (lpSocketInfo->recvBuf != NULL) 
		IOInfo::DeallocateIoInfo(lpSocketInfo->recvBuf);
	if (lpSocketInfo->sendBuf != NULL)
		IOInfo::DeallocateIoInfo(lpSocketInfo->sendBuf);
	lpSocketInfo->socket = INVALID_SOCKET;
	delete lpSocketInfo;
}
