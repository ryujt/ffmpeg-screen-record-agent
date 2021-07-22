#pragma once

#include <Windows.h>
#include <string>
#include <nlohmann/json.hpp>
#include <ffmpeg/VideoCreater.hpp>
#include <AudioCapture.hpp>
#include <DesktopCapture.hpp>
#include <ryulib/debug_tools.hpp>
#include <ryulib/disk.hpp>
#include <ryulib/strg.hpp>

using namespace std;
using namespace nlohmann;

using json = nlohmann::json;

class StartOption {
private:
	StartOption() {};
	static StartOption* obj_;

public:
	VideoCreaterOption option_creater_;
	AudioCaptureOption option_audio_;
	DesktopCaptureOption option_desktop_;

	static StartOption* getObj()
	{
		if (obj_ == nullptr) obj_ = new StartOption();
		return obj_;
	}

	bool parseJson(string option)
	{
		try {
			json_ = json::parse(AnsiToUTF8(option));
			timeout_ = get_json_int(json_, "timeout", -1);

			loadVideoCreaterOption();
			loadAudioCaptureOption();
			loadDesktopCaptureOption();
		} catch (...) {
			return false;
		}
		return true;
	}

	json& item(string name) { return json_[name]; }

	int getTimeOut() { return timeout_; }

private:
	string option_;
	json json_;
	int timeout_ = -1;

	void loadVideoCreaterOption()
	{
		option_creater_.filename = get_json_string(json_, "filename", getExecPath() + "temp.mp4");
		option_creater_.fps = get_json_int(json_, "fps", 25);
		option_creater_.speed = get_json_string(json_, "speed", "slow");
		option_creater_.bitrate = get_json_int(json_, "bitrate-KB", DEFAULT_BITRATE_KB) * 1024;
		option_creater_.gop_size = get_json_int(json_, "gop-size", DEFAULT_GOP_SIZE);
		option_creater_.use_nvenc = get_json_bool(json_, "use-nvenc", false);

		string screen_type = json_["screen-type"].get<string>();

		if (screen_type == "region") {
			option_creater_.width = json_["width"].get<int>();
			option_creater_.height = json_["height"].get<int>();
		}

		if (screen_type == "full") {
			option_creater_.width = GetSystemMetrics(SM_CXSCREEN);
			option_creater_.height = GetSystemMetrics(SM_CYSCREEN);
		}

		if (screen_type == "window") {
			int target_handle = json_["window-handle"].get<int>();
			option_creater_.width = json_["width"].get<int>();
			option_creater_.height = json_["height"].get<int>();
		}
	}

	void loadAudioCaptureOption()
	{
		option_audio_.mic = get_json_int(json_, "mic", -1);
		option_audio_.system_audio = get_json_bool(json_, "system-audio", false);
		option_audio_.volume_mic = get_json_float(json_, "volume-mic", 1.0);
		option_audio_.volume_system = get_json_float(json_, "volume-system", 1.0);
	}

	void loadDesktopCaptureOption()
	{
		option_desktop_.target_handle = -1;
		option_desktop_.fps = get_json_int(json_, "fps", 25);
		option_desktop_.left = 0;
		option_desktop_.top = 0;
		option_desktop_.width = 0;
		option_desktop_.height = 0;
		option_desktop_.with_cursor = true;

		string screen_type = get_json_string(json_, "screen-type", "full");

		if (screen_type == "region") {
			option_desktop_.left = json_["left"].get<int>();
			option_desktop_.top = json_["top"].get<int>();
			option_desktop_.width = json_["width"].get<int>();
			option_desktop_.height = json_["height"].get<int>();
		}

		if (screen_type == "full") {
			option_desktop_.width = GetSystemMetrics(SM_CXSCREEN);
			option_desktop_.height = GetSystemMetrics(SM_CYSCREEN);
		}

		if (screen_type == "window") {
			option_desktop_.target_handle = json_["window-handle"].get<int>();
			option_desktop_.left = json_["left"].get<int>();
			option_desktop_.top = json_["top"].get<int>();
			option_desktop_.width = json_["width"].get<int>();
			option_desktop_.height = json_["height"].get<int>();
		}
	}

	int get_json_int(json j, string name, int default_value)
	{
		try {
			return j[name].get<int>();
		} catch (...) {
			return default_value;
		}
	}

	bool get_json_bool(json j, string name, bool default_value)
	{
		try {
			return j[name].get<bool>();
		} catch (...) {
			return default_value;
		}
	}

	float get_json_float(json j, string name, float default_value)
	{
		try {
			return j[name].get<float>();
		} catch (...) {
			return default_value;
		}
	}

	string get_json_string(json j, string name, string default_value)
	{
		try {
			return j[name].get<string>();
		} catch (...) {
			return default_value;
		}
	}
};

StartOption* StartOption::obj_ = nullptr;