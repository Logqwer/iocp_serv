#include "Packet.h"
#include "def.h"

Packet::TypeMap Packet::typeMap = {
	{typeid(Data), MessageType::DATA},
	{typeid(RoomList), MessageType::ROOMLIST},
	{typeid(RoomInfo), MessageType::ROOM},
	{typeid(Client), MessageType::CLIENT},
	{typeid(PlayState), MessageType::PLAY_STATE},
	{typeid(TransformProto), MessageType::TRANSFORM},
	{typeid(Vector3Proto), MessageType::VECTOR_3},
	{typeid(WorldState), MessageType::WORLD_STATE}
};

Packet::InvTypeMap Packet::invTypeMap = {
	{MessageType::DATA, typeid(Data)},
	{MessageType::ROOMLIST, typeid(RoomList)},
	{MessageType::ROOM, typeid(RoomInfo)},
	{MessageType::CLIENT, typeid(Client)},
	{MessageType::PLAY_STATE, typeid(PlayState)},
	{MessageType::TRANSFORM, typeid(TransformProto)},
	{MessageType::VECTOR_3, typeid(Vector3Proto)},
	{MessageType::WORLD_STATE, typeid(WorldState)}
};

Packet::Packet() 
{
	backup = pack = nullptr;
	backupLength = 0;
	memset(buffer, 0, FOR_IO_SIZE);
}

Packet::~Packet() 
{

}

int Packet::PackMessage(int type, MessageLite* message)
{
	int msgLength = 8;
	if (message != nullptr)
		msgLength += message->ByteSize();

	pack = new char[msgLength];
	aos = new ArrayOutputStream(pack, msgLength);
	cos = new CodedOutputStream(aos);

	if (message == nullptr)
	{
		assert(type != -1);
		cos->WriteLittleEndian32(type);
		cos->WriteLittleEndian32(0);
	}
	else
	{
		Serialize(cos, message);
	}

	CopyMemory(buffer, pack, msgLength);

	delete cos;
	delete aos;
	delete pack;
	return msgLength;
}

void Packet::UnpackHeader(int& type, int& length)
{
	ais = new ArrayInputStream(pack, 8);
	cis = new CodedInputStream(ais);

	cis->ReadLittleEndian32((uint*)&type);
	cis->ReadLittleEndian32((uint*)&length);

	delete cis;
	delete ais;
}

void Packet::UnpackMessage(int& readBytes)
{
	int totalLength = readBytes;
	if (backup != nullptr)
	{
		pack = new char[backupLength + readBytes];
		CopyMemory(pack, backup, backupLength);
		CopyMemory(pack + backupLength, buffer, readBytes);

		totalLength += backupLength;
		delete backup;
		backup = nullptr;
		backupLength = 0;
	}
	else
	{
		pack = new char[readBytes];
		CopyMemory(pack, buffer, readBytes);
	}

	int loop = 0;
	int offset = 0;
	while(offset < totalLength)
	{
		if ((totalLength - offset) < 8)
		{
			BackupStream(offset, totalLength);
			delete pack;
			return;
		}
		
		int type, length;
		UnpackHeader(type, length);
		offset += 8;
		
		MessageContext* msgContext = new MessageContext();
		msgContext->header.type = (int)type;
		msgContext->header.length = (int)length;

		if (length == 0) 
		{
			msgQueue->push(msgContext);
			continue;
		}

		if (length > (totalLength - offset))
		{
			BackupStream(offset, totalLength);
			delete pack;
			return;
		}

		Deserialize(type, length, offset, msgContext->message);
		offset += length;

		msgQueue->push(msgContext);
	}

	delete pack;
}

bool Packet::CheckValidType(int& type)
{
	return invTypeMap.find(type) != invTypeMap.end();
}

void Packet::BackupStream(int& offset, int& totalLength)
{
	int remain = totalLength - offset;
	backup = new char[remain];
	backupLength = remain;
	CopyMemory(backup, pack + offset, remain);
}

void Packet::Serialize(CodedOutputStream*& cos, MessageLite*& message)
{
	int contentType = typeMap[typeid(*message)];
	int contentLength = message->ByteSize();

	//fprintf(stderr, "[ContentType]: %d, [ContentLength]: %d\n", contentType, contentLength);

	cos->WriteLittleEndian32(contentType);
	cos->WriteLittleEndian32(contentLength);
	message->SerializeToCodedStream(cos);
}

void Packet::Deserialize(int& type, int& length, int& offset, MessageLite*& message)
{
	if (!CheckValidType(type)) {
		fprintf(stderr, "Invalid type... %d\n", type);
		return;
	}

	if (type == MessageType::DATA)
	{
		message = new Data();
	}
	else if (type == MessageType::PLAY_STATE)
	{
		message = new PlayState();
	}
	else if (type == MessageType::TRANSFORM)
	{
		message = new TransformProto();
	}
	else if (type == MessageType::VECTOR_3)
	{
		message = new Vector3Proto();
	}
	else if (type == MessageType::WORLD_STATE)
	{
		message = new WorldState();
	}
	else {
		return;
	}

	ais = new ArrayInputStream(pack + offset, length);
	cis = new CodedInputStream(ais);
	bool check = message->ParseFromCodedStream(cis);
	if (check) {
		if (cis->ConsumedEntireMessage()) {
			//printf("ConsumedEntireMessage return true!\n");
		}
		else {
			printf("ConsumedEntireMessage return false!\n");
		}
	}
	else {
		printf("ParseFromCodedStream return false!\n");
	}
	delete cis;
	delete ais;
}

Packet* Packet::AllocatePacket(queue<MessageContext*> *msgQueue)
{
	Packet* lpPacket = new Packet();
	lpPacket->msgQueue = msgQueue;

	return lpPacket;
}

void Packet::DeallocatePacket(Packet* lpPacket)
{
	assert(lpPacket != NULL);
	delete lpPacket;
}