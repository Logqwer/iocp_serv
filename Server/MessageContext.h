#pragma once

class google::protobuf::MessageLite;
using namespace google::protobuf;

struct Header {
	int type;
	int length;

	Header() : type(-1), length(0) {}
};

struct MessageContext {
	Header header;
	MessageLite* message;

	MessageContext() : message(nullptr) {}
};
