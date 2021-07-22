#pragma once

#include <stdio.h>
#include <Windows.h>
#include <turbojpeg.h>
#include <ryulib/base.hpp>
#include <ryulib/strg.hpp>
#include <ryulib/disk.hpp>
#include <ryulib/ThreadQueue.hpp>

class VideoQueue {
private:
	// 메모리로 보관할 bitmap 데이터의 최대 개수
	const static int MAX_BITMAP_COUNT = 4;

	// 최소 여유 메모리 제한 (이보다 더 낮으면 성능 이슈나 다운 위험, 단위 KB)
	const static int MIN_FREE_MEMORY = 1024 * 128;

	const static int MIN_FREE_DISK = 1024 * 128;

public:
	VideoQueue()
	{
		jpeg_zip_ = tjInitCompress();
		jpeg_unzip_ = tjInitDecompress();
	}

	~VideoQueue()
	{
		do_clear();

		if (jpeg_zip_ != nullptr) tjDestroy(jpeg_zip_);
		if (jpeg_unzip_ != nullptr) tjDestroy(jpeg_unzip_);
	}

	void start(int width, int height)
	{
		do_clear();

		width_  = width;
		height_ = height;

		Memory* data = queue_.pop();
		while (data != NULL) {
			delete data;
			data = queue_.pop();
		}

		audio_sync_ = -1;
		audio_mark_ = -1;
	}

	void addBitmap(Memory* data)
	{
		if (has_enough_free_memory() == false) {
			if (on_error_ != nullptr) on_error_(this, ERROR_INSUFFICIENT_MEMORY);
			return;
		}

		if (queue_.size() > MAX_BITMAP_COUNT) {
			if (has_enough_free_disk() == false) {
				if (on_error_ != nullptr) on_error_(this, ERROR_INSUFFICIENT_DISK);
				return;
			}

			try {
				save_bitmap_to_jpeg_file(data);
			} catch (...) {
				if (on_error_ != nullptr) on_error_(this, ERROR_JPEG_CREATE);
			}
		} else {
			data->setTag(audio_mark_);
			queue_.push(data);
		}
	}

	void setAudioSync(int index) 
	{ 
		audio_sync_ = index; 
	}

	void setAudioMark(int index) 
	{ 
		audio_mark_ = index; 
	}

	Memory* getBitmap()
	{
		Memory* data = queue_.front();
		if (data == NULL) return nullptr;
		if (data->getTag() > audio_sync_) return nullptr;

		data = queue_.pop();
		if (data == NULL) return nullptr;

		if (data->getSize() > 0) {
			return data;
		} else {
			try {
				return get_bitmap_from_file(data);
			} catch (...) {
				if (on_error_ != nullptr) on_error_(this, ERROR_JPEG_READ);
				return nullptr;
			}
		}
	}

	void setOnError(IntegerEvent event) { on_error_ = event; }

private:
	ThreadQueue<Memory*> queue_;

	tjhandle jpeg_zip_ = nullptr;
	tjhandle jpeg_unzip_ = nullptr;

	int width_ = 0;
	int height_ = 0;
	int audio_sync_ = -1;
	int audio_mark_ = -1;

	IntegerEvent on_error_ = nullptr;

	void do_clear()
	{
		Memory* data = queue_.pop();
		while (data != NULL) {
			delete data;
			data = queue_.pop();
		}
	}

	void save_bitmap_to_jpeg_file(Memory* data)
	{
		unsigned char* jpeg_buffer = nullptr;
		unsigned long buffer_size = 0;

		int result = tjCompress2(
			jpeg_zip_,
			(const unsigned char*) data->getData(), width_, 0, height_, TJPF_BGRA,
			&jpeg_buffer, &buffer_size,
			TJSAMP_422, 90, TJFLAG_FASTDCT + TJFLAG_BOTTOMUP);
		delete data;

		if (result != 0) return;

		string filename = getTempPath() + getRandomString(64);

		FILE* file;
		fopen_s(&file, filename.c_str(), "wb");
		if (file == NULL) return;

		fwrite(jpeg_buffer, buffer_size, 1, file);
		fclose(file);

		tjFree(jpeg_buffer);

		Memory* jpeg = new Memory(0);
		jpeg->setTag(audio_mark_);
		jpeg->setText(filename);
		queue_.push(jpeg);
	}

	Memory* get_bitmap_from_file(Memory* data)
	{
		string filename = data->getText();
		delete data;

		FILE* file;
		fopen_s(&file, filename.c_str(), "rb");

		if (file == NULL) return nullptr;

		fseek(file, 0, SEEK_END);
		unsigned long buffer_size = ftell(file);

		unsigned char* jpeg_buffer = (unsigned char*) malloc(buffer_size);

		fseek(file, 0, 0);
		fread(jpeg_buffer, buffer_size, 1, file);
		fclose(file);

		remove(filename.c_str());

		Memory* bitmap = new Memory(width_ * height_ * 4);
		int result = tjDecompress2(
			jpeg_unzip_,
			(const unsigned char*) jpeg_buffer, buffer_size,
			(unsigned char*) bitmap->getData(), width_, 0, height_, TJPF_BGRA, TJFLAG_BOTTOMUP);

		if (result != 0) {
			delete bitmap;
			return nullptr;
		}

		return bitmap;
	}

	bool has_enough_free_memory()
	{
		MEMORYSTATUS ms;
		GlobalMemoryStatus(&ms);
		return (ms.dwAvailPhys / 1024) >= MIN_FREE_MEMORY;
	}

	bool has_enough_free_disk()
	{
		char pszDirve[3] = "C:";
		unsigned __int64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;

		if (GetDiskFreeSpaceExA(pszDirve, 
			(PULARGE_INTEGER) &i64FreeBytesToCaller,
			(PULARGE_INTEGER) &i64TotalBytes,
			(PULARGE_INTEGER) &i64FreeBytes) == FALSE) {
			return false;
		}

		return (i64FreeBytesToCaller / 1024) >= MIN_FREE_DISK;
	}
};
