#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

using namespace std;

class FFStream {
public:
	bool open(string filename)
	{
		if (avformat_open_input(&context_, filename.c_str(), NULL, NULL) != 0) return false;
		if (avformat_find_stream_info(context_, NULL) < 0) return false;
		return true;
	}

	void close()
	{
		avformat_close_input(&context_);
		context_ = nullptr;
	}

	AVPacket* read()
	{
		if (context_ == nullptr) return nullptr; 

		AVPacket* packet = av_packet_alloc();

		if (av_read_frame(context_, packet) < 0) {
			av_packet_free(&packet);
			return nullptr;
		}

		return packet;
	}

	void move(int ms)
	{
		int duration = context_->duration * 1000 / AV_TIME_BASE;
		int target = ms;
		int frameNumber = av_rescale(ms, context_->streams[0]->time_base.den, context_->streams[0]->time_base.num);
		frameNumber = frameNumber / 1000;
		avformat_seek_file(context_, 0, 0, frameNumber, duration, AVSEEK_FLAG_FRAME);
	}

	AVFormatContext* getContext()
	{
		return context_;
	}

private:
	AVFormatContext* context_ = nullptr;
};