#pragma once

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/message_lite.h>

#include <unordered_map>
#include <queue>
#include <typeindex>
#include <typeinfo>
#include <cassert>
#include <cstring>

#include "MessageContext.h"
#include "ErrorHandle.h"
#include "protobuf/room.pb.h"
#include "protobuf/PlayState.pb.h"
#include "protobuf/data.pb.h"

using std::queue;
using namespace packet;
using namespace state;
using namespace google::protobuf;
using namespace google::protobuf::io;

#define FOR_IO_SIZE 4096
#define FOR_BAKCUP_SIZE 2048
#define FOR_PACK_SIZE 4096
#define MAX_SIZE 4096

class Packet {
public:
	Packet();
	~Packet();

	int PackMessage(int type = -1, MessageLite* message = nullptr);
	void UnpackMessage(int& totalLength);

public:
	static Packet* AllocatePacket(queue<MessageContext*> *msgQueue);
	static void DeallocatePacket(Packet* lpPacket);

public:
	char buffer[FOR_IO_SIZE];

private:
	ArrayInputStream* ais;
	ArrayOutputStream* aos;
	CodedInputStream* cis;
	CodedOutputStream* cos;

	char* backup;
	char* pack;
	int backupLength;

	queue<MessageContext*> *msgQueue;

private:
	typedef std::unordered_map<std::type_index, int> TypeMap;
	typedef std::unordered_map<int, std::type_index> InvTypeMap;
	static TypeMap typeMap;
	static InvTypeMap invTypeMap;

private:
	void UnpackHeader(int& type, int& length);

	bool CheckValidType(int& type);
	void BackupStream(int& offset, int& readBytes);

	void Serialize(CodedOutputStream*&, MessageLite*&);
	void Deserialize(int& type, int& length, int& offset, MessageLite*& message);
};