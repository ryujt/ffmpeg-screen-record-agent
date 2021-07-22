# 시작 옵션 파싱 처리

``` cpp
	void loadVideoCreaterOption()
	{
		option_creater_.filename = get_json_string(json_, "filename", getExecPath() + "temp.mp4");
		option_creater_.fps = get_json_int(json_, "fps", 25);
		option_creater_.speed = get_json_string(json_, "speed", "slow");
		option_creater_.bitrate = get_json_int(json_, "bitrate-KB", DEFAULT_BITRATE_KB) * 1024;
		option_creater_.use_nvenc = get_json_bool(json_, "use-nvenc", false);

		string screen_type = json_["screen-type"].get<string>();  // 지정안되면 오류가 납니다.

		if (screen_type == "region") {
			option_creater_.width  = json_["width"].get<int>();   // 지정안되면 오류가 납니다.
			option_creater_.height = json_["height"].get<int>();  // 지정안되면 오류가 납니다.
		}

		if (screen_type == "full") {
			option_creater_.width = GetSystemMetrics(SM_CXSCREEN);
			option_creater_.height = GetSystemMetrics(SM_CYSCREEN);
		}

		if (screen_type == "window") {
			int target_handle = json_["window-handle"].get<int>();  // 지정안되면 오류가 납니다.
			tagRECT rect;
			if (GetWindowRect((HWND) target_handle, &rect)) {
				option_creater_.width = rect.right - rect.left;
				option_creater_.height = rect.bottom - rect.top;
			}
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
		option_desktop_.fps = get_json_int(json_, "fps", 25);

		string screen_type = get_json_string(json_, "screen-type", "full");

		if (screen_type == "region") {
			option_desktop_.left   = json_["left"].get<int>();    // 지정안되면 오류가 납니다.
			option_desktop_.top    = json_["top"].get<int>();     // 지정안되면 오류가 납니다.
			option_desktop_.width  = json_["width"].get<int>();   // 지정안되면 오류가 납니다.
			option_desktop_.height = json_["height"].get<int>();  // 지정안되면 오류가 납니다.
		}

		if (screen_type == "full") {
			option_desktop_.width = GetSystemMetrics(SM_CXSCREEN);
			option_desktop_.height = GetSystemMetrics(SM_CYSCREEN);
		}

		if (screen_type == "window") {
			option_desktop_.target_handle = json_["window-handle"].get<int>();  // 지정안되면 오류가 납니다.
			tagRECT rect;
			if (GetWindowRect((HWND) option_desktop_.target_handle, &rect)) {
				option_desktop_.left = rect.left;
				option_desktop_.top = rect.top;
				option_desktop_.width = rect.right - rect.left;
				option_desktop_.height = rect.bottom - rect.top;
			}
		}
	}
```