#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <Windows.h>
#include <unordered_map>
#include <utility>
#include "def.h"
#include "SocketInfo.h"
#include "protobuf/room.pb.h"
#include "protobuf/data.pb.h"
#include "Room.h"

class ServerManager {
public:
	void Start(int port=PORT);
	void Stop();

	bool SendPacket(SocketInfo* lpSocketInfo, const MessageContext* msgContext);
	bool RecvPacket(SocketInfo* lpSocketInfo);

	static ServerManager& getInstance() {
		if (self == nullptr) {
			self = new ServerManager();
		}
		return *self;
	}

private:
	static ServerManager* self;
	static unsigned __stdcall ThreadMain(void* pVoid);

private:
	ServerManager();
	~ServerManager();

	void InitSocket(int port, int prime = 2, int sub = 2);
	void InitCompletionPort(int maxNumberOfThreads = 0);
	void AcceptClient();
	void CloseClient(SocketInfo* lpSocketInfo, bool graceful = false);
	void CreateThreadPool(int numOfThreads = 0);
	void ShutdownThreads();

	bool HandleSendEvent(SocketInfo* lpSocketInfo, DWORD dwBytesTransferred);
	bool HandleRecvEvent(SocketInfo* lpSocketInfo, DWORD dwBytesTransferred);
	bool HandleWithoutBody(SocketInfo* lpSocketInfo, int& type);
	bool HandleWithBody(SocketInfo* lpSocketInfo, MessageLite* message, int& type);

	//Temperary Method
	void InitRoom(RoomInfo* pRoom, SocketInfo* lpSocketInfo, string& roomName, int& limits, string& userName);
	void SendInitData(SocketInfo*);
	void ProcessDisconnection(SocketInfo* lpSocketInfo);

private:
	WSAData wsaData;
	HANDLE hCompPort;
	SOCKET servSock;

	int threadPoolSize;
	HANDLE hMutexObj;

	int roomIdStatus;
	RoomList roomList;
	// <RoomId, RoomContext> 
	std::unordered_map<int, Room*> serverRoomList;
	// <RoomName, RoomId> 
	std::unordered_map<string, int> roomTable;
	// <SocketInfo, Client>
	std::unordered_map<SocketInfo*, Client*> clientLocationTable;

	std::unordered_map<int, int> checkCall;
  
	CRITICAL_SECTION csForRoomList;
	CRITICAL_SECTION csForServerRoomList;
	CRITICAL_SECTION csForRoomTable;
	CRITICAL_SECTION csForClientLocationTable;
};
