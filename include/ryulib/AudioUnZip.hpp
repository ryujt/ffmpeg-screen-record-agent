#ifndef AUDIOUNZIP_HPP
#define AUDIOUNZIP_HPP


#include <ryulib/AudioZipUtils.hpp>
#include <ryulib/AudioIO.hpp>
#include <ryulib/AudioDecoder.hpp>
#include <ryulib/debug_tools.hpp>

using namespace std;

class AudioUnZip {
public:
	/** AudioUnZip 생성자
	@param channels 오디오의 채널 수. 1: 모노, 2: 스테레오
	@param sampe_rate 오디오의 sampling rate. 초당 캡쳐할 샘플링(오디오의 데이터) 개수
	*/
	AudioUnZip(int channels, int sampe_rate)
		: audio_output_(channels, sampe_rate, FRAMES_PER_BUFFER), audio_decoder_(channels, sampe_rate)
	{
		audio_output_.setOnError(
			[&](const void* obj, int error_code) {
				if (OnError_ != nullptr) OnError_(this, error_code);
			}
		);
		audio_output_.open();

		audio_decoder_.setOnError(
			[&](const void* obj, int error_code) {
				if (OnError_ != nullptr) OnError_(this, error_code);
			}
		);
		audio_decoder_.setOnDecode(
			[&](const void* obj, const void* data, int size) {
				audio_output_.play(data, size);
			}
		);
	}

	~AudioUnZip()
	{
		audio_output_.close();
	}

	/** 압축된 데이터를 압축해제하고 재생을 한다.
	@param data 압축된 오디오 데이터의 주소
	@param size 압축된 오디오 데이터의 크기
	*/
	void play(const void* data, int size) 
	{
		audio_decoder_.add(data, size);
	}

	/** 주어진 숫자만큼 버퍼 앞부분에서 오디오 패킷을 삭제한다.
	음성 송수신 등에 의해서 발생한 딜레이를 제거하기 위해서 사용한다.
	@param count 스킵할 오디오 데이터 개수
	*/
	void skip(int count=1)
	{
		audio_output_.skip(count);
	}

	/** 출력이 끝나지 않은 패킷의 갯수 */
	int getDelayCount() { return audio_output_.getDelayCount(); }

	/** 볼륨상태를 알려준다. 
	@return 1.0은 원음 크기를 의미하고 0.5는 50%의 볼륨이다.
	*/
	float getVolume() 
	{ 
		return audio_output_.getVolume(); 
	}

	/** 볼륨 크기를 결정한다.
	@param value 스피커를 통해서 출력 될 음량의 크기를 결정한다. 1.0은 원음 크기를 의미하고 0.5는 50%의 볼륨이다.
	*/
	void setVolume(float value) 
	{
		audio_output_.setVolume(value);
	}

	/** OnError 이벤트 핸들러를 지정한다.
	@param event 에러가 났을 때 실행될 이벤트 핸들러
	*/
	void setOnError(IntegerEvent event) { OnError_ = event; }

private:
	AudioOutput audio_output_; 
	AudioDecoder audio_decoder_;
	IntegerEvent OnError_ = nullptr;
};


#endif  // AUDIOUNZIP_HPP