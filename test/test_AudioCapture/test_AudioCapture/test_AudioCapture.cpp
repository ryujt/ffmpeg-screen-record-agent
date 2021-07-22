#include <Windows.h>
#include <iostream>
#include <AudioCapture.hpp>

int main()
{
    AudioCaptureOption option;
    option.system_audio = true;

    AudioCapture audioCapture;
    audioCapture.start(option);

    while (true) {
        Memory* data = audioCapture.getAudioData();
        if (data != nullptr) {
            delete data;
        } else {
            Sleep(1);
        }
    }
}