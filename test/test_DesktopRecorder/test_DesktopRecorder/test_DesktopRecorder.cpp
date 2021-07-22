#include <Windows.h>
#include <iostream>
#include <string>
#include <DesktopRecorder.hpp>
#include <StartOption.hpp>
#include <ryulib/strg.hpp>

int main()
{
	string screen_type = "";
	HWND handle = FindWindow(L"Notepad", NULL);
	string option_str =
		string("{	\"code\": \"start\", ") +
		//string("	\"filename\" : \"D:/Work/녹화/test01.mp4\", ") +
		string("	\"filename\" : \"F:/test01.mp4\", ") +
		string("	\"mic\" : -1, ") +
		string("	\"system-audio\" : true, ") +
		string("	\"volume-mic\" : 1.0, ") +
		string("	\"volume-system\" : 1.0, ") +
		string("	\"screen-type\" : \"%s\", ") +
		string("	\"window-handle\" : %d, ") +
		string("	\"fps\" : 24, ") +
		string("	\"speed\" : \"veryfast\", ") +
		string("	\"bitrate-KB\" : 1024, ") +
		string("	\"use-nvenc\" : false, ") +
		string("	\"left\" : 0, ") +
		string("	\"top\" : 0, ") +
		string("	\"width\" : 1920, ") +
		string("	\"height\" : 1080, ") +
		string("	\"with-cursor\" : true }");

	DesktopRecorder recorder;
	recorder.setOnPacketsInBuffer([](const void* sender, int packets) {
		//printf("packets: %d \n", packets);
	});

	while (true) {
		string line;
		printf("(r)egion, (f)ull, (w)indow, (s)top, (p)ause, r(e)sume, (q)uit: ");
		getline(cin, line);

		if (line == "r") {
			StartOption::getObj()->parseJson( format_string(option_str, "region", handle) );
		} else if (line == "f") {
			StartOption::getObj()->parseJson(format_string(option_str, "full", handle));
		} else if (line == "w") {
			StartOption::getObj()->parseJson(format_string(option_str, "window", handle));
		}

		string temp = "rfw";
		if (temp.find(line) != string::npos) {
			if (recorder.start() == false) printf("녹화를 시작할 수 없습니다. \n");
		}

		if (line == "s") {
			recorder.stop();
		}

		if (line == "p") {
			recorder.pause();
		}

		if (line == "e") {
			recorder.resume();
		}

		if (line == "q") {
			recorder.terminateAndWait();
			break;
		}
	}
}
