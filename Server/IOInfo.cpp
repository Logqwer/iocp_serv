#include "IOInfo.h"
#include <cassert>

IOInfo::IOInfo()
{
	memset(&(overlapped), 0, sizeof(WSAOVERLAPPED));
	wsaBuf.len = 0;
	wsaBuf.buf = NULL;
	called = false;
	hSemaForSend = CreateSemaphore(NULL, 1, 1, NULL);
}

IOInfo::~IOInfo() 
{
	if (hSemaForSend != NULL)
		CloseHandle(hSemaForSend);
}

IOInfo* IOInfo::AllocateIoInfo()
{
	IOInfo* lpIoInfo = new IOInfo();
	lpIoInfo->lpPacket = Packet::AllocatePacket(&(lpIoInfo->msgQueue));
	lpIoInfo->wsaBuf.buf = lpIoInfo->lpPacket->buffer;
	lpIoInfo->wsaBuf.len = FOR_IO_SIZE;

	return lpIoInfo;
}

void IOInfo::DeallocateIoInfo(IOInfo* lpIoInfo)
{
	assert(lpIoInfo != NULL);
	if (lpIoInfo->lpPacket != NULL)
		Packet::DeallocatePacket(lpIoInfo->lpPacket);
	free(lpIoInfo);
}

bool IOInfo::Receive(const SOCKET& sock)
{
	if (called) {
		fprintf(stderr, "Already Recv Called!!\n");
		return true;
	}

	DWORD dwRecvBytes = 0;
	DWORD dwFlags = 0;

	ZeroMemory(&overlapped, sizeof(WSAOVERLAPPED));
	int rtn = WSARecv(sock, &wsaBuf, 1, &dwRecvBytes, &dwFlags, &overlapped, NULL);
	if (rtn == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			ErrorHandling("[Socket #%d] WSARecv Failed...", errCode, false);
			return false;
		}
	}

	called = true;
	return true;
}

bool IOInfo::Send(const SOCKET& sock, const MessageContext* msgContext)
{
	WaitForSingleObject(hSemaForSend, INFINITE);

	if (msgContext != nullptr) {
		ZeroMemory(wsaBuf.buf, FOR_IO_SIZE);
		wsaBuf.len = lpPacket->PackMessage(msgContext->header.type, msgContext->message);
	}

	DWORD dwSendBytes = 0;
	DWORD dwFlags = 0;

	ZeroMemory(&overlapped, sizeof(WSAOVERLAPPED));
	int rtn = WSASend(sock, &wsaBuf, 1, &dwSendBytes, dwFlags, &overlapped, NULL);
	if (rtn == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			ErrorHandling("WSASend Failed...", errCode, false);
			return false;
		}
	}
	return true;
}

void IOInfo::HandleReceive(int readBytes)
{
	if (readBytes <= 0)
		return;

	lpPacket->UnpackMessage(readBytes);
	ZeroMemory(wsaBuf.buf, FOR_IO_SIZE);
}

bool IOInfo::HandleSend(const SOCKET& sock)
{
	long previous;
	ReleaseSemaphore(hSemaForSend, 1, NULL);
	return true;
}

void IOInfo::CopyRawToBuffer(const void* src, DWORD& length)
{
	wsaBuf.len = length;
	ZeroMemory(wsaBuf.buf, FOR_IO_SIZE);
	CopyMemory(wsaBuf.buf, src, length);
}

void IOInfo::CopyBufferToRaw(void* dst, DWORD& length)
{
	CopyMemory(dst, wsaBuf.buf, length);
	ZeroMemory(wsaBuf.buf, FOR_IO_SIZE);
}

bool IOInfo::HasMessage()
{
	return !msgQueue.empty();
}

MessageContext* IOInfo::NextMessage()
{
	if (msgQueue.empty())
		return nullptr;

	MessageContext* msg = msgQueue.front();
	msgQueue.pop();
	return msg;
}
