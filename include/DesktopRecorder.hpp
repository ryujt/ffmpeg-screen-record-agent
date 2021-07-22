#pragma once

#include <Global.hpp>
#include <StartOption.hpp>
#include <VideoEncoder.hpp>
#include <MediaBuffer.hpp>
#include <windows.h>
#include <atomic>
#include <ryulib/base.hpp>

using namespace std;

enum DesktopRecorderState {Started, Stoped, Paused};

class DesktopRecorder {
public:
	DesktopRecorder()
	{
		video_encoder_ = new VideoEncoder();

		video_encoder_->setOnNeedAudio([&]() {
			if (video_encoder_ == nullptr) return false;
			if (media_buffer_ == nullptr) return false;

			check_idle_count();

			Memory* audio = media_buffer_->getAudio();
			if (audio == nullptr) return false;

			video_encoder_->writeAudioPacket(audio);

			delete audio;
			return true;
		});

		video_encoder_->setOnNeedVideo([&]() {
			if (video_encoder_ == nullptr) return false;
			if (media_buffer_ == nullptr) return false;

			check_idle_count();

			Memory* bitmap = media_buffer_->getBitmap();

			video_encoder_->writeBitmap(bitmap);

			if (bitmap == nullptr) return false;

			delete bitmap;
			return true;
		});

		video_encoder_->setOnError([&](const void* obj, int error_code) {
			if (on_error_ != nullptr) on_error_(this, error_code);
		});

		media_buffer_ = new MediaBuffer();

		media_buffer_->setOnError([&](const void* obj, int error_code) {
			if (on_error_ != nullptr) on_error_(this, error_code);
		});
	}

	int start()
	{
		keepAlive();

		if (state_ == DesktopRecorderState::Started) {
			if (on_error_ != nullptr) on_error_(this, ERROR_RECODER_IS_STARTED);
			return ERROR_RECODER_IS_STARTED;
		}

		if (state_ == DesktopRecorderState::Paused) {
			resume();
			return 0;
		}

		int result = media_buffer_->start();
		if (result != 0) {
			state_ = DesktopRecorderState::Stoped;
			return result;
		}

		result = video_encoder_->start();
		if (result != 0) {
			media_buffer_->stop();
			state_ = DesktopRecorderState::Stoped;
			return result;
		}

		state_ = DesktopRecorderState::Started;

		return 0;
	}

	void stop()
	{
		if (state_ == DesktopRecorderState::Stoped) return;
		state_ = DesktopRecorderState::Stoped;

		media_buffer_->stop();
		video_encoder_->stop();
		keepAlive();
	}

	void pause()
	{
		keepAlive();

		if (state_ != DesktopRecorderState::Started) return;
		state_ = DesktopRecorderState::Paused;

		media_buffer_->pause();
	}

	void resume()
	{
		keepAlive();

		if (state_ != DesktopRecorderState::Paused) return;
		state_ = DesktopRecorderState::Started;

		media_buffer_->resume();
	}

	void terminate()
	{
		media_buffer_->terminate();
		video_encoder_->terminate();
	}

	void terminateNow()
	{
		media_buffer_->terminateNow();
		video_encoder_->terminateNow();
	}

	void terminateAndWait()
	{
		VideoEncoder* video_encoder = video_encoder_;
		video_encoder_ = nullptr;
		video_encoder->terminateAndWait();
		delete video_encoder;

		MediaBuffer* media_buffer = media_buffer_;
		media_buffer_ = nullptr;
		media_buffer->terminateAndWait();
		delete media_buffer;
	}

	/** 메인 프로그램에서 ping 메시지를 주기적으로 보내서 메인 프로그램이 동작 중임을 알린다.
	일정 시간 이상 메시지가 없으면 녹화를 종료하고 녹화 에이전트를 종료한다.
	*/
	void keepAlive()
	{
		old_tick_ = 0;
		idle_count_ms_ = 0;
	}

	int getStatus()
	{
		if ((state_ != DesktopRecorderState::Paused) && (media_buffer_->getPacketsInBuffer() > 0)) return DesktopRecorderState::Started;
		return state_;
	}

	void setOnError(IntegerEvent event) { on_error_ = event; }
	void setOnPacketsInBuffer(IntegerEvent event) { media_buffer_->setOnPacketsInBuffer(event); }
	void setOnCompleted(NotifyEvent event) { video_encoder_->setOnCompleted(event); }

private:
	VideoEncoder* video_encoder_ = nullptr;
	MediaBuffer* media_buffer_ = nullptr;

	DesktopRecorderState state_ = DesktopRecorderState::Stoped;
	
	atomic<int> idle_count_ms_ = 0;
	DWORD old_tick_ = 0;

	IntegerEvent on_error_ = nullptr;

	void check_idle_count()
	{
		if (StartOption::getObj()->getTimeOut() <= 0) {
			keepAlive();
			return;
		}

		DWORD tick = GetTickCount();
		if ((old_tick_ == 0) || (tick < old_tick_)) {
			old_tick_ = tick;
			return;
		}

		idle_count_ms_.fetch_add(tick - old_tick_);

		if (idle_count_ms_ > (StartOption::getObj()->getTimeOut() * 1000)) {
			if (on_error_ != nullptr) on_error_(this, ERROR_PING_TIMEOUT);
		}

		old_tick_ = tick;
	}
};
