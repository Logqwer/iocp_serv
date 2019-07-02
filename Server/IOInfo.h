#pragma once

#include "Packet.h"
#include <WinSock2.h>
#include <Windows.h>
#include <queue>
using std::queue;

class IOInfo {
public:
	IOInfo();
	~IOInfo();

public:
	static IOInfo* AllocateIoInfo();
	static void DeallocateIoInfo(IOInfo* lpIoInfo);

public:
	bool Receive(const SOCKET& sock);
	bool Send(const SOCKET& sock, const MessageContext* msgContext);
	void HandleReceive(int readBytes);
	bool HandleSend(const SOCKET& sock);

	void CopyRawToBuffer(const void* src, DWORD& length);
	void CopyBufferToRaw(void* dst, DWORD& length);

	bool HasMessage();
	MessageContext* NextMessage();

private:
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	Packet* lpPacket;

	queue<MessageContext*> msgQueue;

	HANDLE hSemaForSend;

public:
	bool called;
	MessageContext* ptr;
};