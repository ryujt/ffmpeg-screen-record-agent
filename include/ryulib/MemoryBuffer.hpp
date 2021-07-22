#ifndef MEMORYBUFFER_HPP
#define MEMORYBUFFER_HPP

#include <queue>
#include <ryulib/base.hpp>

using namespace std;

class Buffer {
public:
	Buffer(const void* data, int size)
	{
		size_ = size;
		if (size > 0) {
			data_ = malloc(size);
			memcpy(data_, data, size);
		}
		else {
			data_ = nullptr;
		}
		offset_ = (char*) data_;
	}

	~Buffer()
	{
		if (data_ != nullptr) {
			free(data_);
			data_ = nullptr;
			offset_ = nullptr;
		}
		size_ = 0;
	}

	int read(void* data, int size)
	{
		if (size_ <= 0) return 0;

		int result = size;
		if (result > size_) result = size_;
		size_ = size_ - result;

		memcpy(data, offset_, result);
		offset_ = offset_ + result;

		return result;
	}

private:
	void* data_;
	char* offset_;
	int size_;
};

class MemoryBuffer {
public:
	~MemoryBuffer()
	{
		clear();
	}

	void clear()
	{
		while (queue_.empty() == false) {
			Buffer* buffer = queue_.front();
			queue_.pop();
			delete buffer;
		}
		size_ = 0;
	}

	void write(const void* data, int size)
	{
		queue_.push(new Buffer(data, size));
		size_ = size_ + size;
	}

	void* read(int size)
	{
		if (size > size_) return nullptr;

		size_ = size_ - size;

		int bytes_to_copy = size;
		void* result = malloc(size);
		char* offset = (char*) result;

		while (queue_.empty() == false) {
			Buffer* buffer = queue_.front();
			int copied = buffer->read(offset, bytes_to_copy);
			if (copied <= 0) {
				queue_.pop();
				delete buffer;
				continue;
			}

			bytes_to_copy = bytes_to_copy - copied;
			offset = offset + copied;

			if (bytes_to_copy <= 0) break;
		}

		return result;
	}

private:
	int size_ = 0;
	queue<Buffer*> queue_;
};


#endif  // MEMORYBUFFER_HPP