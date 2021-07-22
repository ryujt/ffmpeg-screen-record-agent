#pragma once

#include <StartOption.hpp>
#include <AudioCapture.hpp>
#include <DesktopCapture.hpp>
#include <VideoQueue.hpp>
#include <ryulib/base.hpp>

using namespace std;

class MediaBuffer {
private:
	const static int INTERVAL_AUDIO_COUNT_LIMIT = 10;

public:
	MediaBuffer()
	{
		desktop_capture_ = new DesktopCapture();

		audio_capture_ = new AudioCapture();

		audio_capture_->setOnAudioMark_([&](const void* sender, int index) {
			video_queue_->setAudioMark(index);
		});

		desktop_capture_->setOnData([&](const void* sender, const Memory* data) {
			video_queue_->addBitmap((Memory*)data);
		});

		audio_capture_->setOnError([&](const void* obj, int error_code) {
			if (on_error_ != nullptr) on_error_(this, error_code);
		});

		video_queue_ = new VideoQueue();
		video_queue_->setOnError([&](const void* obj, int error_code) {
			if (on_error_ != nullptr) on_error_(this, error_code);
		});
	}

	int start()
	{
		if (audio_capture_->start(StartOption::getObj()->option_audio_) == false) return ERROR_START_AUDIO;

		video_queue_->start(StartOption::getObj()->option_desktop_.getBitmapWidth(), StartOption::getObj()->option_desktop_.getBitmapHeight());
		desktop_capture_->start(StartOption::getObj()->option_desktop_);

		return 0;
	}

	void stop()
	{
		audio_capture_->stop();
		desktop_capture_->stop();
	}

	void pause()
	{
		audio_capture_->pause();
		desktop_capture_->stop();
	}

	void resume()
	{
		audio_capture_->resume();
		desktop_capture_->start(StartOption::getObj()->option_desktop_);
	}

	void terminate()
	{
		audio_capture_->stop();
		desktop_capture_->terminate();
	}

	void terminateNow()
	{
		audio_capture_->stop();
		desktop_capture_->terminateNow();
	}

	void terminateAndWait()
	{
		AudioCapture* audio_capture = audio_capture_;
		audio_capture_ = nullptr;
		audio_capture->terminateAndWait();
		delete audio_capture;

		DesktopCapture* desktop_capture = desktop_capture_;
		desktop_capture_ = nullptr;
		desktop_capture->terminateAndWait();
		delete desktop_capture;

		VideoQueue* video_queue = video_queue_;
		video_queue_ = nullptr;
		delete video_queue;
	}

	Memory* getAudio()
	{
		if (audio_capture_ == nullptr) return nullptr;
		if (video_queue_ == nullptr) return nullptr;

		Memory* data = audio_capture_->getAudioData();
		fire_packets_in_buffer_event();
		if (data != nullptr) video_queue_->setAudioSync(data->getTag());
		return data;
	}

	int getPacketsInBuffer()
	{
		return audio_capture_->getPacketsInBuffer();
	}

	Memory* getBitmap()
	{
		if (video_queue_ == nullptr) return nullptr;

		return video_queue_->getBitmap();
	}

	void setOnError(IntegerEvent event) { on_error_ = event; }
	void setOnPacketsInBuffer(IntegerEvent event) { on_packets_in_buffer_ = event; }

private:
	AudioCapture* audio_capture_ = nullptr;
	DesktopCapture* desktop_capture_ = nullptr;
	VideoQueue* video_queue_ = nullptr;

	// 인코딩 안된 패킷 개수를 일정 간격으로 이벤트로 알리기 위한 변수
	int interval_audio_count_ = 0;

	IntegerEvent on_error_ = nullptr;
	IntegerEvent on_packets_in_buffer_ = nullptr;

	void fire_packets_in_buffer_event()
	{
		interval_audio_count_++;
		if (interval_audio_count_ >= INTERVAL_AUDIO_COUNT_LIMIT) {
			interval_audio_count_ = 0;
			if (on_packets_in_buffer_ != nullptr)
				on_packets_in_buffer_(this, audio_capture_->getPacketsInBuffer());
		}
	}
};
