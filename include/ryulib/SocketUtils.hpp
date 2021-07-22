#ifndef RYULIB_SOCKETUTILS_HPP
#define RYULIB_SOCKETUTILS_HPP

#include <cstring>
#include <string>
#include <functional>

const int HEADER_SIZE = 3;
const int MAX_CONNECTION = 4096;
const int PACKET_LIMIT = 32 * 1024;

const int ERROR_CONNECT = -1;
const int ERROR_READ = -2;
const int ERROR_WRITE = -3;

using namespace std;

#pragma pack(push, 1)
typedef struct _Packet {
	unsigned short packet_size;
	char packet_type;
	char data_start;

	void* getData() { return &data_start; }
	int getDataSize() { return packet_size - 3; }

	char* getText()
	{
		char* result = (char*) malloc(packet_size);
		memset(result, 0, packet_size);
		memcpy(result, &data_start, getDataSize());
		return result;
	}
} Packet;
#pragma pack(pop)

static Packet* create_packet(char packet_type, const void* data, int size)
{
	Packet* packet = (Packet*) malloc(size + sizeof(Packet) - 1);
	if (packet == NULL) return nullptr;

	packet->packet_type = packet_type;
	packet->packet_size = size + sizeof(Packet) - 1;
	if (size > 0) memcpy(&packet->data_start, data, size);

	return packet;
}

static Packet* create_packet(char packet_type, string text)
{
	Packet* packet = (Packet*) malloc(text.size() + sizeof(Packet) - 1);
	if (packet == NULL) return nullptr;

	packet->packet_type = packet_type;
	packet->packet_size = text.size() + sizeof(Packet) - 1;
	if (text.size() > 0) memcpy(&packet->data_start, text.c_str(), text.size());

	return packet;
}

class PacketReader {
public:
	PacketReader()
	{
		buffer_ = (char*) malloc(PACKET_LIMIT * 2);
		data_ = (char*) malloc(PACKET_LIMIT + 4);
	}

	~PacketReader()
	{
		free(buffer_);
		free(data_);
	}

	void clear()
	{
		size_ = 0;
	}

	void write(void* data, int size)
	{
		if (size <= 0) return;

		Packet* packet_ = (Packet*) data;
		char* tail = buffer_ + size_;
		size_ = size_ + size;
		memcpy(tail, data, size);
	}

	Packet* read()
	{
		if (canRead() == false) return nullptr;

		Packet* packet_ = (Packet*) buffer_;

		memcpy(data_, buffer_, packet_->packet_size);

		size_ = size_ - packet_->packet_size;
		if (size_ > 0) {
			char* tail = buffer_ + packet_->packet_size;
			memcpy(buffer_, tail, size_);
		}

		return (Packet*) data_;
	}

	bool canRead()
	{
		if (buffer_ == nullptr) return false;
		if (size_ < HEADER_SIZE) return false;

		Packet* packet_ = (Packet*) buffer_;
		return packet_->packet_size <= size_;
	}

private:
	char* buffer_ = nullptr;
	char* data_ = nullptr;
	int size_ = 0;

};

#endif  // RYULIB_SOCKETUTILS_HPP