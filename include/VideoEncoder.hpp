#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <Global.hpp>
#include <StartOption.hpp>
#include <ffmpeg/VideoCreater.hpp>
#include <ryulib/base.hpp>
#include <ryulib/debug_tools.hpp>
#include <ryulib/Scheduler.hpp>

using namespace std;
using namespace ryulib;

typedef function<bool()> NeedAudioEvent;
typedef function<bool()> NeedVideoEvent;

class VideoEncoder {
private:
	const static int TASK_START = 1;
	const static int TASK_STOP = 2;

public:
	VideoEncoder()
	{
		scheduler_.setOnRepeat([&]() {
			try {
				do_repeat();
			} catch (...) {
				is_started_ = false;
				delete videoCreater_;
				videoCreater_ = nullptr;
				free(bitmap_);
				bitmap_ = nullptr;

				if (on_error_ != nullptr) on_error_(this, ERROR_ENCODING);

				DebugOutput::trace("VideoEncoder - scheduler_.setOnRepeat error");
			}
		});

		scheduler_.setOnTask([&](int task, const string text, const void* data, int size, int tag) {
			switch (task) {
				case TASK_START: do_start((VideoCreater*) data); break;
				case TASK_STOP: do_stop(); break;
			}
		});
	}

	~VideoEncoder()
	{
		do_stop();
		if (videoCreater_ != nullptr) delete videoCreater_;
		videoCreater_ = nullptr;
	}

	int start()
	{
		bool old = is_started_.exchange(true);
		if (old) return ERROR_RECODER_IS_STARTED;

		VideoCreater* videoCreater = new VideoCreater();
		if (videoCreater->open(StartOption::getObj()->option_creater_) == false) {
			is_started_ = false;
			delete videoCreater;
			if (on_error_ != nullptr) on_error_(this, ERROR_CREATE_ENCODER);
			DebugOutput::trace("VideoEncoder.do_start - VideoCreater error");
			return ERROR_CREATE_VIDEO_FILE;
		}

		scheduler_.add(TASK_START, videoCreater);

		return 0;
	}

	void stop()
	{
		scheduler_.add(TASK_STOP);
	}

	void terminate()
	{
		scheduler_.terminate();
		do_stop();
	}

	void terminateNow()
	{
		scheduler_.terminateNow();
		do_stop();
	}

	void terminateAndWait()
	{
		scheduler_.terminateAndWait();
		do_stop();
	}

	void writeBitmap(Memory* packet)
	{
		if (bitmap_ == nullptr) return;
		if (packet != nullptr) memcpy(bitmap_, packet->getData(), packet->getSize());
		if (videoCreater_->writeBitmap(bitmap_) == false) {
			if (on_error_ != nullptr) {
				on_error_(this, ERROR_IN_WRITTING_STREAM_FILE);
			}
		}
	}

	void writeAudioPacket(Memory* packet)
	{
		if (videoCreater_->do_writeAudioPacket(packet->getData(), packet->getSize()) == false) {
			if (on_error_ != nullptr) {
				on_error_(this, ERROR_IN_WRITTING_STREAM_FILE);
			}
		}
	}

	void setOnNeedAudio(NeedAudioEvent event) { on_need_audio_ = event; }
	void setOnNeedVideo(NeedVideoEvent event) { on_need_video_ = event; }
	void setOnError(IntegerEvent event) { on_error_ = event; }
	void setOnCompleted(NotifyEvent event) { on_completed_ = event; }

private:
	Scheduler scheduler_;
	VideoCreater* videoCreater_ = nullptr;

	atomic<bool> is_started_ = false;
	void* bitmap_ = nullptr;

	NeedAudioEvent on_need_audio_ = nullptr;
	NeedVideoEvent on_need_video_ = nullptr;
	IntegerEvent on_error_ = nullptr;
	NotifyEvent on_completed_ = nullptr;

	void do_start(VideoCreater* videoCreater)
	{
		videoCreater_ = videoCreater;

		if (bitmap_ != nullptr) free(bitmap_);
		bitmap_ = malloc(videoCreater_->getBitmapSize());
		memset(bitmap_, 0, videoCreater_->getBitmapSize());

		scheduler_.start();
	}

	void do_stop()
	{
		scheduler_.stop();

		if (videoCreater_ == nullptr) return;

		while (true) {
			if (on_need_audio_ == nullptr) break;
			if (on_need_video_ == nullptr) break;

			if (videoCreater_->isVideoTurn()) {
				on_need_video_();
			}
			else {
				if (on_need_audio_() == false) break;
			}
		}

		is_started_ = false;

		delete videoCreater_;
		videoCreater_ = nullptr;

		free(bitmap_);
		bitmap_ = nullptr;

		if (on_completed_ != nullptr) on_completed_(this);
	}

	void do_repeat()
	{
		if (videoCreater_ == nullptr) return;
		if (on_need_audio_ == nullptr) return;
		if (on_need_video_ == nullptr) return;

		bool need_sleep = false;
		if (videoCreater_->isVideoTurn()) {
			if (on_need_video_() == false) need_sleep = true;
		}
		else {
			if (on_need_audio_() == false) need_sleep = true;
		}

		if (need_sleep) scheduler_.sleep(1);
	}
};