#pragma once

#include <Global.hpp>
#include <WASAPI/SystemAudio.hpp>
#include <ffmpeg/AudioResampling.hpp>
#include <ryulib/MemoryBuffer.hpp>
#include <ryulib/ThreadQueue.hpp>
#include <ryulib/SimpleThread.hpp>

using namespace std;
using namespace ryulib;

class SystemAudioCaptrue {
public:
	SystemAudioCaptrue()
	{
		system_audio_.setOnData([&](const void* sender, const void* data, int size) {
			if (data == nullptr) return;
			if (queue_out_.size() > 2) return;

			queue_in_.push(new Memory(data, size));
		});

		audio_resampling_.setOnData([&](const void* sender, const void* data, int size) {
			if (data != nullptr) queue_out_.push(new Memory(data, size));
		});

		thread_ = new SimpleThread([&](SimpleThread* simpleThread) {
			while (simpleThread->isTerminated() == false) {
				Memory* data = queue_in_.pop();
				if (data == NULL) {
					simpleThread->sleep(1);
					continue;
				}

				buffer_.write(data->getData(), data->getSize());
				delete data;

				void* src = buffer_.read(audio_resampling_.getSrcBufferSize());
				if (src != nullptr) {
					audio_resampling_.execute(src, audio_resampling_.getSrcBufferSize());
					free(src);
				}
			}
		});
	}

	~SystemAudioCaptrue()
	{
		thread_->terminateAndWait();
		do_clear();
	}

	void start()
	{
		do_clear();

		system_audio_.open();
		audio_resampling_.open(
			system_audio_.getChannels(),
			system_audio_.getSamples(),
			system_audio_.getBitsPerSample() / 8,
			AUDIO_CHANNEL, AUDIO_SAMPLE_RATE, AUDIO_SAMPLE_SIZE, AUDIO_FRAMES_PER_BUFFER
		);
	}

	void stop()
	{
		system_audio_.close();
		audio_resampling_.close();
	}

	Memory* getAudioData()
	{
		Memory* memory = queue_out_.pop();
		//printf("SystemAudioCaptrue delay: %d \r", queue_out_.size());
		if (memory == NULL) return nullptr;
		return memory;
	}

private:
	SystemAudio system_audio_;
	MemoryBuffer buffer_;
	AudioResampling audio_resampling_;
	SimpleThread* thread_;
	ThreadQueue<Memory*> queue_out_;
	ThreadQueue<Memory*> queue_in_;

	void do_clear()
	{
		buffer_.clear();

		Memory* data = queue_in_.pop();
		while (data != NULL) {
			delete data;
			data = queue_in_.pop();
		}

		data = queue_out_.pop();
		while (data != NULL) {
			delete data;
			data = queue_out_.pop();
		}
	}
};