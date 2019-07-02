#include <process.h>
#include "def.h"
#include "Room.h"
#include "ServerManager.h"


Room::Room(RoomInfo * initVal) : roomInfo(initVal)
{
	InitializeCriticalSection(&csForClientSockets);
	InitializeCriticalSection(&csForRoomInfo);
	InitializeCriticalSection(&csForBroadcast);
	gameStarted = false;
}

Room::~Room()
{
	DeleteCriticalSection(&csForClientSockets);
	DeleteCriticalSection(&csForRoomInfo);
	DeleteCriticalSection(&csForBroadcast);
	CloseHandle(hCompPort);

	std::cout << "~Room() called" << std::endl;
}

void Room::AddClientInfo(SocketInfo * lpSocketInfo, string& userName)
{
	EnterCriticalSection(&csForClientSockets);
	clientSockets.push_front(lpSocketInfo);
	LeaveCriticalSection(&csForClientSockets);
	clientMap[userName] = lpSocketInfo;
}

void Room::RemoveClientInfo(SocketInfo * lpSocketInfo, string& userName)
{
	EnterCriticalSection(&csForClientSockets);
	clientSockets.remove(lpSocketInfo);
	LeaveCriticalSection(&csForClientSockets);
	if (clientMap.find(userName) != clientMap.end()) {
		clientMap.erase(userName);
	}
}

std::forward_list<SocketInfo*>::const_iterator Room::ClientSocketsBegin()
{
	EnterCriticalSection(&csForClientSockets);
	auto itr = clientSockets.cbegin();
	LeaveCriticalSection(&csForClientSockets);
	return itr;
}

std::forward_list<SocketInfo*>::const_iterator Room::ClientSocketsEnd()
{
	EnterCriticalSection(&csForClientSockets);
	auto itr = clientSockets.cend();
	LeaveCriticalSection(&csForClientSockets);
	return itr;
}

void Room::InsertDataIntoBroadcastQueue(DWORD additionalData, ULONG_PTR message)
{
	PostQueuedCompletionStatus(hCompPort, additionalData, message, NULL);
}

void Room::ProcessReadyEvent(Client*& affectedClient)
{
	EnterCriticalSection(&csForRoomInfo);
	bool toReady = !affectedClient->ready() ? true : false;
	if (toReady) 
	{
		roomInfo->set_readycount(roomInfo->readycount() + 1);
	} 
	else 
	{
		roomInfo->set_readycount(roomInfo->readycount() - 1);
	}	
	affectedClient->set_ready(toReady);
	LeaveCriticalSection(&csForRoomInfo);
}

Client* Room::ProcessTeamChangeEvent(Client*& affectedClient)
{
	bool isOnRedTeam = affectedClient->position() < BLUEINDEXSTART ? true : false;
	
	int maxuser = roomInfo->limit() / 2;
	int nextIdx;
	Client* newClient;
	EnterCriticalSection(&csForRoomInfo);
	if (isOnRedTeam) 
	{
		nextIdx = roomInfo->blueteam_size();
		if (nextIdx == maxuser)
			return nullptr;
		
		nextIdx += BLUEINDEXSTART;
		newClient = MoveClientToOppositeTeam(affectedClient, nextIdx, roomInfo->mutable_redteam(), roomInfo->mutable_blueteam());
	}
	else 
	{
		nextIdx = roomInfo->redteam_size();
		if (nextIdx == maxuser)
			return nullptr;

		newClient = MoveClientToOppositeTeam(affectedClient, nextIdx, roomInfo->mutable_blueteam(), roomInfo->mutable_redteam());
	}
	LeaveCriticalSection(&csForRoomInfo);
	return newClient;
}

bool Room::ProcessLeaveGameroomEvent(Client*& affectedClient, SocketInfo* lpSocketInfo) 
{
	int position = affectedClient->position();
	bool isOnRedTeam = position < BLUEINDEXSTART ? true : false;
	bool isClosed = false;

	EnterCriticalSection(&csForRoomInfo);
	if (affectedClient->ready())
		roomInfo->set_readycount(roomInfo->readycount() - 1);

	if (isOnRedTeam) 
		roomInfo->mutable_redteam()->DeleteSubrange(position, 1);
	else
		roomInfo->mutable_blueteam()->DeleteSubrange(position % BLUEINDEXSTART, 1);

	EnterCriticalSection(&csForClientSockets);
	clientSockets.remove(lpSocketInfo); 
	LeaveCriticalSection(&csForClientSockets);

	if (roomInfo->current() == 1)
	{
		isClosed = true; 
	}
	else
	{
		if (roomInfo->host() == position)
		{ 
			ChangeGameroomHost(isOnRedTeam); 
		}
	}

	AdjustClientsIndexes(position);
	roomInfo->set_current(roomInfo->current() - 1); 
	LeaveCriticalSection(&csForRoomInfo);
	return isClosed;
}

bool Room::CanStart(string& errorMessage)
{	
	EnterCriticalSection(&csForRoomInfo);
	bool allReady = (roomInfo->current() - 1) == roomInfo->readycount();
	bool isFair = roomInfo->blueteam_size() == roomInfo->redteam_size();
	LeaveCriticalSection(&csForRoomInfo);
	
	if (!allReady)
	{
		errorMessage = "To start a game, all users should be ready!";
		return false;
	}
	
	if (!isFair) 
	{
		errorMessage = "To start a game, the number of users on each team should be the same!";
		return false;
	}

	return true;
}

void Room::SetGameStartFlag(bool to)
{
	gameStarted = to;
}

bool Room::HasGameStarted() const
{
	return gameStarted;
}

SocketInfo*& Room::GetSocketUsingName(string & userName)
{
	return clientMap[userName];
}

void Room::InitCompletionPort(int maxNumberOfThreads)
{
	hCompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, maxNumberOfThreads);
	if (hCompPort == NULL) {
		ErrorHandling(WSAGetLastError());
		return;
	}
}

void Room::CreateThreadPool(int numOfThreads)
{
	for (int i = 0; i < numOfThreads; ++i) {
		DWORD dwThreadId = 0;
		HANDLE hThread = BEGINTHREADEX(NULL, 0, Room::ThreadMain, this, 0, &dwThreadId);
		CloseHandle(hThread);
	}
}

unsigned __stdcall Room::ThreadMain(void * pVoid)
{
	printf("[Room Thread #%d]\n", GetCurrentThreadId());

	Room* self = (Room*)pVoid;
	MessageLite* pMessage;
	LPOVERLAPPED lpOverlapped;

	DWORD dwBytesTransferred = 0;
	ServerManager& servManager = ServerManager::getInstance();

	while (true)
	{
		pMessage = NULL;
		lpOverlapped = NULL;
	
		bool rtn = GetQueuedCompletionStatus(self->hCompPort, &dwBytesTransferred,
			reinterpret_cast<ULONG_PTR*>(&pMessage), &lpOverlapped, INFINITE);

		if (!rtn) {
			if (lpOverlapped != NULL) {
				printf("lpOverlapped is not NULL!: %d\n", GetLastError());
			}
			else {
				printf("lpOverlapped is NULL!: %d\n", GetLastError());
			}
			//continue;
			break;
		}

		if (pMessage == NULL) {
			ErrorHandling("Message is NULL....", false);
			continue;
		}
		else if (reinterpret_cast<DWORD>(pMessage) == KILL_THREAD)
		{
			std::cout << "ACTION : KILL THREAD" << std::endl;
			break;
		}

		// Broadcast
		EnterCriticalSection(&self->csForBroadcast);
		auto begin = self->ClientSocketsBegin();
		auto end = self->ClientSocketsEnd();

		switch (dwBytesTransferred)
		{
			case DISPOSABLE:
			case NON_DISPOSABLE:
				self->BroadcastGeneralData(servManager, dwBytesTransferred, pMessage, begin, end);
				break;
			case TYPEWITHOUTBODY:
				self->BroadcastTypeData(servManager, pMessage, begin, end);
				break;
			default:
				self->BroadcastRawData(servManager, dwBytesTransferred, reinterpret_cast<char*>(pMessage), begin, end);
				break;
		}
		LeaveCriticalSection(&self->csForBroadcast);
	}
	return 0;
}

Client* Room::GetClient(int position)
{
	return position < BLUEINDEXSTART ? 
		roomInfo->mutable_redteam(position) : roomInfo->mutable_blueteam(position % BLUEINDEXSTART);
}

Client* Room::MoveClientToOppositeTeam(Client*& affectedClient, int next_pos, Mutable_Team deleteFrom, Mutable_Team addTo)
{
	Client* newClient = addTo->Add();
	newClient->CopyFrom(*affectedClient);
	int prev_pos = affectedClient->position();
	if (affectedClient->position() == roomInfo->host()) 
	{
		roomInfo->set_host(next_pos);
	}
	deleteFrom->DeleteSubrange(prev_pos % BLUEINDEXSTART, 1);
	AdjustClientsIndexes(prev_pos);
	newClient->set_position(next_pos);
	return newClient;
}

void Room::AdjustClientsIndexes(int basePos)
{ 
	int size;
	Mutable_Team team;
	bool needHostDecrement = false;
	int hostPos = roomInfo->host();

	if (basePos < BLUEINDEXSTART)
	{
		if (hostPos < BLUEINDEXSTART && hostPos >= basePos)
			needHostDecrement = true;
		size = roomInfo->redteam_size();
		team = roomInfo->mutable_redteam();
	}
	else
	{
		if (hostPos >= BLUEINDEXSTART && hostPos >= basePos)
			needHostDecrement = true;
		basePos -= BLUEINDEXSTART;
		size = roomInfo->blueteam_size();
		team = roomInfo->mutable_blueteam();
	}

	if (basePos >= size)
		return;

	for (int i = basePos; i < size; i++)
	{
		(*team)[i].set_position(team->Get(i).position() - 1);
	}

	if (needHostDecrement)
		roomInfo->set_host(roomInfo->host() - 1);
}

void Room::ChangeGameroomHost(bool isOnRedteam)
{
	int nextHost;
	if (isOnRedteam) 
	{
		if (roomInfo->blueteam_size() != 0)
			nextHost = BLUEINDEXSTART;
		else
			nextHost = 0;
	}
	else
	{
		if (roomInfo->redteam_size() != 0)
			nextHost = 0;
		else
			nextHost = BLUEINDEXSTART;
	}
	roomInfo->set_host(nextHost);
	Client* nextHostClnt = GetClient(nextHost);
	if (nextHostClnt->ready())
	{
		GetClient(nextHost)->set_ready(false);
		roomInfo->set_readycount(roomInfo->readycount() - 1);
	}
}

void Room::BroadcastGeneralData(ServerManager& servManager, DWORD broadcastType, MessageLite * data, SocketIterator begin, SocketIterator end)
{
	MessageContext msgContext;
	msgContext.message = data;
	for (auto itr = begin; itr != end; itr++)
	{
		if ((*itr)->socket != INVALID_SOCKET)
		{
			if (!servManager.SendPacket(*itr, &msgContext))
				std::cout << "Send Message Failed\n";
		}
	}

	if (broadcastType == DISPOSABLE)
		delete data;
}

void Room::BroadcastTypeData(ServerManager& servManager, MessageLite * data, SocketIterator begin, SocketIterator end)
{
	int* type = (int*)data;
	MessageContext msgContext;
	msgContext.header.type = *type;
	for (auto itr = begin; itr != end; itr++)
	{
		if ((*itr)->socket != INVALID_SOCKET)
		{
			if (!servManager.SendPacket(*itr, &msgContext))
				std::cout << "Send Message Failed\n";
		}
	}
	delete type;
}

void Room::BroadcastRawData(ServerManager& servManager, DWORD dwBytesTransferred, char * data, SocketIterator begin, SocketIterator end)
{
	for (auto itr = begin; itr != end; itr++)
	{
		if ((*itr)->socket != INVALID_SOCKET)
		{
			(*itr)->sendBuf->CopyRawToBuffer(data, dwBytesTransferred);
			if (!servManager.SendPacket(*itr, nullptr))
				std::cout << "Send Message Failed\n";
		}
	}
	delete data;
}
