// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <portaudio.h>
#include <ryulib/debug_tools.hpp>
#include <ryulib/strg.hpp>

#pragma comment(lib, "portaudio")

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) int initAudioDevice()
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        DebugOutput::trace("ERROR: Pa_Initialize returned 0x%x\n", err);
    }
    return err;
}

extern "C" __declspec(dllexport) int getDeviceCount()
{
    return Pa_GetDeviceCount();
}

extern "C" __declspec(dllexport) void getDeviceName(int index, char* name, int size)
{
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    string ansi_name = UTF8ToANSI(deviceInfo->name);
    memset(name, 0, size);
    if (ansi_name.length() < size) size = ansi_name.length();
    memcpy(name, ansi_name.c_str(), size);
}

extern "C" __declspec(dllexport) int getInputChannels(int index)
{
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    return deviceInfo->maxInputChannels;
}

extern "C" __declspec(dllexport) int getOutputChannels(int index)
{
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    return deviceInfo->maxOutputChannels;
}