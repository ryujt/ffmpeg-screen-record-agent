#ifndef RYULIB_BASE_HPP
#define RYULIB_BASE_HPP

#include <stdlib.h>
#include <string>
#include <cstring>
#include <functional>

using namespace std;

namespace ryulib {

class Memory;

typedef function<void()> VoidEvent;
typedef function<void(const void*)> NotifyEvent;
typedef function<void(const void*, const string)> StringEvent;
typedef function<void(const void*, int code, const string)> ErrorEvent;
typedef function<void(const void*, int)> IntegerEvent;
typedef function<void(const const const void*, Memory*)> MemoryEvent;
typedef function<void(const void*, const void*, int)> DataEvent;
typedef function<bool(const void*)> AskEvent;
typedef function<void(int, const string, const void*, int, int)> TaskEvent;

/** 메모리 할당을 받아서 포인터와 크기를 함께 묶어서 사용하는 데이터 클래스
*/
class Memory {
public:
	Memory()
	{
		data_ = nullptr;
		size_ = 0;
	}

	Memory(int size)
	{
		size_ = size;
		if (size > 0) {
			data_ = malloc(size);
			memset(data_, 0, size);
		} else {
			data_ = nullptr;
		}
	}

	Memory(const void* data, int size)
	{
		if ((size <= 0) || (data == nullptr)) {
			data_ = nullptr;
			size_ = 0;
			return;
		}

		size_ = size;
		data_ = malloc(size);
		memcpy(data_, data, size);
	}

	~Memory()
	{
		if (data_ != nullptr) {
			free(data_);
			data_ = nullptr;
		}
		size_ = 0;
	}

	void loadMemory(const void* src, int size)
	{
		memcpy(data_, src, size);
	}

	void* getData() { return data_; }
	int getSize() { return size_; }
	void* getUserData() { return user_data_; }
	int getTag() { return tag_; }
	string getText() { return text_; }

	void setUserData(const void* user_data) { user_data_ = (void*) user_data; }
	void setTag(int tag) { tag_ = tag; }
	void setText(string text) { text_ = text; }

private:
	void* data_ = nullptr;
	int size_ = 0;
	void* user_data_ = nullptr;
	int tag_ = 0;
	string text_;
};

/** 포인터와 기타 정보를 함께 묶어서 사용하는 데이터 클래스
*/
class MemoryBox {
public:
	MemoryBox(void* data, int size, void* user_data, int tag)
		: data_(data), size_(size), user_data_(user_data), tag_(tag)
	{}
private:
	void* data_ = nullptr;
	int size_ = 0;
	void* user_data_ = nullptr;
	int tag_ = 0;
};

}
#endif  // RYULIB_BASE_HPP
