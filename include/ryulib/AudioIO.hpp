#ifndef AUDIOIO_HPP
#define AUDIOIO_HPP

#include <math.h>
#include <portaudio.h>
#include <ryulib/base.hpp>
#include <ryulib/ThreadQueue.hpp>
#include <ryulib/debug_tools.hpp>

using namespace std;
using namespace ryulib;

static bool is_audio_inited = false;

const int ERROR_NO_DEFAULT_INPUT_DEVICE = -1;
const int ERROR_OPEN_INPUT_DEVICE = -2;
const int ERROR_START_INPUT_DEVICE = -3;

const int ERROR_NO_DEFAULT_OUTPUT_DEVICE = -4;
const int ERROR_OPEN_OUTPUT_DEVICE = -5;
const int ERROR_START_OUTPUT_DEVICE = -6;

class Audio {
public:
	/** 오디오 장치를 초기화한다. AudioIO를 사용하기 전에 반드시 호출되어야 한다.
	@return 정상처리되면 true가 리턴된다.
	*/
	static bool init()
	{
		if (is_audio_inited) return true;
		PaError err = paNoError;

		err = Pa_Initialize();
		if (err != paNoError) {
			DebugOutput::trace("Error: Pa_Initialize. \n");
			return false;
		}

		is_audio_inited = true;
		return true;
	}
};

class AudioInput {
public:
	/** AudioInput 생성자
	@param channels 캡쳐할 오디오의 채널 수. 1: 모노, 2: 스테레오
	@param sampe_rate 캡쳐할 오디오의 sampling rate. 초당 캡쳐할 샘플링(오디오의 데이터) 개수
	@param smaple_size 캡쳐할 오디오의 포멧. 2: 16bit (paInt16), 4: 32bit (paFloat32)
	@param fpb 한 번에 처리할 프레임의 갯수
	*/
	AudioInput(int channels, int sampe_rate, int smaple_size, int fpb)
		: channels_(channels), sampe_rate_(sampe_rate), smaple_size_(smaple_size), 
		fpb_(fpb), buffer_size_(smaple_size * fpb * channels)
	{
	}

	/** 오디오 장치를 오픈
	@param device_id 오디오를 캡쳐할 디바이스 아이디 -1은 기본 입력 장치
	@return 에러 코드가 리턴된다. 정상처리되면 0이 리턴된다.
	*/
	int open(int device_id = -1) 
	{
		PaError err = paNoError;
		PaStreamParameters inputParameters;

		if (device_id == -1) {
			inputParameters.device = Pa_GetDefaultInputDevice();
		}
		else {
			inputParameters.device = device_id;
		}

		if (inputParameters.device == paNoDevice) {
			DebugOutput::trace("Error: No default input device. \n");
			if (OnError_ != nullptr) OnError_(this, ERROR_NO_DEFAULT_INPUT_DEVICE);
			return ERROR_NO_DEFAULT_INPUT_DEVICE;
		}

		switch (smaple_size_) {
			case 2: inputParameters.sampleFormat = paInt16; break;
			case 4: inputParameters.sampleFormat = paFloat32; break;
			default: throw "smaple_size는 2(16bit)와 4(32bit)만 지정이 가능합니다.";
		}

		const PaDeviceInfo* info = Pa_GetDeviceInfo(inputParameters.device);
		if (info == nullptr) {
			DebugOutput::trace("Error: AudioInput- can't find audio device.\n");
			if (OnError_ != nullptr) OnError_(this, ERROR_OPEN_INPUT_DEVICE);
			return ERROR_OPEN_INPUT_DEVICE;
		}

		inputParameters.channelCount = channels_;
		inputParameters.suggestedLatency = info->defaultLowInputLatency;
		inputParameters.hostApiSpecificStreamInfo = NULL;

		err = Pa_OpenStream(&stream_, &inputParameters, NULL, sampe_rate_, fpb_, paClipOff, recordCallback, this);
		if (err != paNoError) {
			DebugOutput::trace("Error: AudioInput- Pa_OpenStream \n");
			if (OnError_ != nullptr) OnError_(this, ERROR_OPEN_INPUT_DEVICE);
			return ERROR_OPEN_INPUT_DEVICE;
		}

		err = Pa_StartStream(stream_);
		if (err != paNoError) {
			DebugOutput::trace("Error: AudioInput - Pa_StartStream \n");
			if (OnError_ != nullptr) OnError_(this, ERROR_START_INPUT_DEVICE);
			return ERROR_START_INPUT_DEVICE;
		}

		return 0;
	}

	/** 오디오 장치를 닫는다. 오디오 캡쳐가 중단된다. */
	void close() 
	{
		if (stream_ != nullptr) {
			Pa_CloseStream(stream_);
			stream_ = nullptr;
		}
	}

	/** 오디오가 캡쳐되는 중인지 알려준다.
	@return 오디오 캡쳐 중, false: 오디오 캡쳐가 중단됨
	*/
	bool isActive()
	{
		return Pa_IsStreamActive(stream_) == 1;
	}

	/** 볼륨상태를 알려준다. 
	@return 1.0은 원음 크기를 의미하고 0.5는 50%의 볼륨이다.
	*/
	float getVolume() 
	{ 
		return volume_; 
	}

	/** 볼륨 크기를 결정한다.
	@param value 마이크에서 입력된 음량의 크기를 결정한다. 1.0은 원음 크기를 의미하고 0.5는 50%의 볼륨이다.
	*/
	void setVolume(float value) 
	{
		volume_ = value;
	}

	/** OnError 이벤트 핸들러를 지정한다.
	@param event 에러가 났을 때 실행될 이벤트 핸들러
	*/
	void setOnError(IntegerEvent event) { OnError_ = event; }

	/** OnData 이벤트 핸들러를 지정한다.
	@param event 오디오가 캡쳐되었을 때 실행될 이벤트 핸들러
	*/
	void setOnData(const DataEvent &value) { on_data_ = value; }

private:
	static int recordCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, 
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) 
	{
		AudioInput *audio_input = (AudioInput *) userData;

		if (abs(audio_input->volume_ - 1.0) > 0.0001) {
			float* data = (float*) inputBuffer;
			for (int i = 0; i < audio_input->fpb_; i++) {
				*data = (*data) * audio_input->volume_;
				data++;
			}
		}

		if (audio_input->on_data_ != nullptr) audio_input->on_data_(audio_input, inputBuffer, audio_input->buffer_size_);
		return paContinue;
	}

	int channels_;
	int sampe_rate_;
	int smaple_size_;
	int fpb_;
	int buffer_size_;

	float volume_ = 1.0;

	PaStream* stream_ = nullptr;
	IntegerEvent OnError_ = nullptr;
	DataEvent on_data_ = nullptr;
};

class AudioOutput {
public:
	/** AudioOutput 생성자
	@param channels 오디오의 채널 수. 1: 모노, 2: 스테레오
	@param sampe_rate 오디오의 sampling rate.
	@param fpb 한 번에 처리할 프레임의 갯수
	*/
	AudioOutput(int channels, int sampe_rate, int smaple_size, int fpb)
		: channels_(channels), sampe_rate_(sampe_rate), smaple_size_(smaple_size), 
		fpb_(fpb), buffer_size_(smaple_size * fpb * channels)
	{
		DebugOutput::trace("AudioOutput - buffer_size_: %d \n", buffer_size_);

		mute_ = malloc(buffer_size_);
		memset(mute_, 0, buffer_size_);
	}

	/** 오디오 장치를 오픈
	@param device_id 오디오를 출력할 디바이스 아이디 -1은 기본 입력 장치
	@return 에러 코드가 리턴된다. 정상처리되면 0이 리턴된다.
	*/
	int open(int device_id = -1)
	{
		PaError err = paNoError;
		PaStreamParameters outputParameters;

		if (device_id == -1) {
			outputParameters.device = Pa_GetDefaultOutputDevice();
		}
		else {
			outputParameters.device = device_id;
		}

		if (outputParameters.device == paNoDevice) {
			DebugOutput::trace("Error: AudioOutput - paNoDevice \n");
			if (OnError_ != nullptr) OnError_(this, ERROR_NO_DEFAULT_OUTPUT_DEVICE);
			return ERROR_NO_DEFAULT_OUTPUT_DEVICE;
		}

		switch (smaple_size_) {
		case 2: outputParameters.sampleFormat = paInt16; break;
		case 4: outputParameters.sampleFormat = paFloat32; break;
		default: throw "smaple_size는 2(16bit)와 4(32bit)만 지정이 가능합니다.";
		}

		outputParameters.channelCount = channels_;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;

		err = Pa_OpenStream(&stream_, NULL, &outputParameters, sampe_rate_, fpb_, paClipOff, playCallback, this);
		if (err != paNoError) {
			DebugOutput::trace("Error: AudioOutput - Pa_OpenStream \n");
			if (OnError_ != nullptr) OnError_(this, ERROR_OPEN_OUTPUT_DEVICE);
			return ERROR_OPEN_OUTPUT_DEVICE;
		}

		err = Pa_StartStream(stream_);
		if (err != paNoError) {
			DebugOutput::trace("Error: AudioOutput - Pa_StartStream \n");
			if (OnError_ != nullptr) OnError_(this, ERROR_START_OUTPUT_DEVICE);
			return ERROR_START_OUTPUT_DEVICE;
		}

		return 0;
	}

	/** 오디오 장치를 닫는다. */
	void close()
	{
		Pa_CloseStream(stream_);
	}

	/** 오디오를 재생한다.
	@param data 재생할 오디오 데이터
	@param size 재생할 오디오 데이터의 크기
	*/
	void play(const void *data, int size)
	{
		Memory* memory = new Memory(data, size);
		queue_.push(memory);
	}

	/** 주어진 숫자만큼 버퍼 앞부분에서 오디오 패킷을 삭제한다.
	음성 송수신 등에 의해서 발생한 딜레이를 제거하기 위해서 사용한다.
	@param count 스킵할 오디오 데이터 개수
	*/
	void skip(int count)
	{
		for (int i=0; i<count; i++) {
			Memory* memory;
			if (queue_.pop(memory) == false) break;
			delete memory;
		}
	}

	/** 오디오 출력 장치를 사용할 수 있는가? */
	bool isActive() { return Pa_IsStreamActive(stream_) == 1; }

	/** 출력이 끝나지 않은 패킷의 갯수 */
	int getDelayCount() { return queue_.size(); }

	/** 볼륨상태를 알려준다. 
	@return 1.0은 원음 크기를 의미하고 0.5는 50%의 볼륨이다.
	*/
	float getVolume() 
	{ 
		return volume_; 
	}

	/** 볼륨 크기를 결정한다.
	@param value 스피커를 통해서 출력 될 음량의 크기를 결정한다. 1.0은 원음 크기를 의미하고 0.5는 50%의 볼륨이다.
	*/
	void setVolume(float value) 
	{
		volume_ = value;
	}

	/** OnError 이벤트 핸들러를 지정한다.
	@param event 에러가 났을 때 실행될 이벤트 핸들러
	*/
	void setOnError(IntegerEvent event) { OnError_ = event; }

private:
	static int playCallback(const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData) 
	{
		AudioOutput *audio_output = (AudioOutput *) userData;

		Memory* memory;
		if (audio_output->queue_.pop(memory)) {
			memcpy(outputBuffer, memory->getData(), memory->getSize());
			delete memory;

			if (abs(audio_output->volume_ - 1.0) > 0.0001) {
				float* data = (float*) outputBuffer;
				for (int i = 0; i < audio_output->fpb_; i++) {
					*data = (*data) * audio_output->volume_;
					data++;
				}
			}
		} else {
			memcpy(outputBuffer, audio_output->mute_, audio_output->buffer_size_);
		}
		return paContinue;
	}

	int channels_;
	int sampe_rate_;
	int smaple_size_;
	int fpb_;
	int buffer_size_;
	void *mute_;

	float volume_ = 1.0;

	PaStream *stream_;
	ThreadQueue<Memory*> queue_;
	IntegerEvent OnError_ = nullptr;
};


#endif  // AUDIOIO_HPP