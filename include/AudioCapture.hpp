#pragma once

#include <Global.hpp>
#include <SystemAudioCaptrue.hpp>
#include <ryulib/base.hpp>
#include <ryulib/debug_tools.hpp>
#include <ryulib/AudioIO.hpp>
#include <ryulib/ThreadQueue.hpp>
#include <ryulib/Timer.hpp>

typedef struct AudioCaptureOption {
	int mic = -1;
	bool system_audio = false;
	float volume_mic = 1.0;
	float volume_system = 1.0;
};

class AudioCapture {
private:
	const static int AUDIO_INTERVAL_MS = (1000 * AUDIO_FRAMES_PER_BUFFER / AUDIO_SAMPLE_RATE) * 1000;

public:
	AudioCapture()
	{
		silent_audio_ = new Memory(AUDIO_FRAMES_SIZE);

		Audio::init();

		mic_ = new AudioInput(AUDIO_CHANNEL, AUDIO_SAMPLE_RATE, AUDIO_SAMPLE_SIZE, AUDIO_FRAMES_PER_BUFFER);

		mic_->setOnError([&](const void* sender, int error_code) {
			do_error(mic_, error_code);
		});

		mic_->setOnData([&](const void* obj, const void* buffer, int buffer_size) {
			on_data(mic_, buffer, buffer_size);
		});

		timer_.setOnTimer([&](const void* sender) {
			on_data(nullptr, nullptr, 0);
		});
	}

	~AudioCapture()
	{
		delete mic_;
		delete silent_audio_;
	}

	void terminateAndWait()
	{
		stop();
		timer_.terminateAndWait();
	}

	bool start(AudioCaptureOption option)
	{
		system_audio_.start();

		option_ = option; 
		index_ = 0;
		if (option.mic == 999) {
			timer_.start(AUDIO_INTERVAL_MS);
			return true;
		} else {
			return mic_->open(option.mic) == 0;
		}
	}

	void stop()
	{
		mic_->close();
		timer_.stop();
		system_audio_.stop();
	}

	void pause()
	{
		stop();
	}

	void resume()
	{
		system_audio_.start();

		if (option_.mic == 999) {
			timer_.start(AUDIO_INTERVAL_MS);
		} else {
			if (mic_->open(option_.mic) != 0) timer_.start(AUDIO_INTERVAL_MS);
		}
	}

	Memory* getAudioData()
	{
		Memory* data = queue_.pop();
		if (data == NULL) return nullptr;
		return data;
	}

	int getPacketsInBuffer() { return queue_.size(); }

	void setOnError(IntegerEvent event) { on_error_ = event; }
	void setOnAudioMark_(IntegerEvent event) { on_audio_mark_ = event; }

private:
	AudioInput* mic_ = nullptr;
	ThreadQueue<Memory*> queue_;
	SystemAudioCaptrue system_audio_;
	Timer timer_;

	// 충분한 크기의 묵음 처리용 데이터
	Memory* silent_audio_;

	AudioCaptureOption option_;
	int index_ = 0;

	IntegerEvent on_audio_mark_ = nullptr;
	IntegerEvent on_error_ = nullptr;

	void do_error(AudioInput* sender, int error_code)
	{
		if (on_error_ != nullptr) on_error_(this, ERROR_AUDIO_OPEN);
	}

	void on_data(AudioInput* sender, const void* buffer, int buffer_size)
	{
		Memory* data = new Memory(AUDIO_FRAMES_SIZE);

		void* mic_audio;
		if (buffer == nullptr) {
			mic_audio = silent_audio_->getData();
		} else {
			mic_audio = (void*) buffer;
		}

		Memory* system_audio = system_audio_.getAudioData();
		if ((option_.system_audio == false) || (system_audio == nullptr)) {
			if (system_audio != nullptr) delete system_audio;
			system_audio = silent_audio_;
		}

		float* mic = (float*) mic_audio;
		float* system = (float*) system_audio->getData();
		float* dst = (float*) data->getData();
		for (int i = 0; i < (AUDIO_FRAMES_SIZE / sizeof(float)); i++) {
			*dst = *mic * option_.volume_mic + *system * option_.volume_system;
			mic++;
			system++;
			dst++;
		}

		if (system_audio != silent_audio_) delete system_audio;

		index_++;
		data->setTag(index_);
		queue_.push(data);
		if (on_audio_mark_ != nullptr) on_audio_mark_(this, index_);
	}
};
