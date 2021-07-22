#pragma once

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <thread>
#include <ryulib/base.hpp>
#include <ryulib/debug_tools.hpp>

using namespace ryulib;

#define SAFE_RELEASE(punk) \
  if ((punk) != nullptr) { \
    (punk)->Release();     \
    (punk) = nullptr;      \
  }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);

class SystemAudio {
private:
	const static int FRAME_SIZE = 1920;
	const static int CAPTURE_BUFFER_SIZE = 7680;
	const static int REFTIMES_PER_SEC = 10000000;
	const static int REFTIMES_PER_MILLISEC = 10000;

	const static int TASK_OPEN = 1;
	const static int TASK_CLOSE = 2;

public:
	SystemAudio()
	{
		CoInitialize(NULL);
	}

	~SystemAudio()
	{
		CoUninitialize();
	}

	bool open()
	{
		hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**) &pEnumerator);
		if (FAILED(hr)) {
			DebugOutput::trace("CoCreateInstance() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
		if (FAILED(hr)) {
			DebugOutput::trace("GetDefaultAudioEndpoint() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**) &pAudioClient);
		if (FAILED(hr)) {
			DebugOutput::trace("Activate() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pAudioClient->GetMixFormat(&pwfx);
		if (FAILED(hr)) {
			DebugOutput::trace("GetMixFormat() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC, 0, pwfx, NULL);
		if (FAILED(hr)) {
			DebugOutput::trace("Initialize() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
		if (FAILED(hr)) {
			DebugOutput::trace("GetBufferSize() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**) &pCaptureClient);
		if (FAILED(hr)) {
			DebugOutput::trace("GetService() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		hr = pAudioClient->Start();
		if (FAILED(hr)) {
			DebugOutput::trace("Start() error = %d , hr = 0x%08x", GetLastError(), hr);
			fin_values();
			return false;
		}

		DebugOutput::trace("wFormatTag: %d", pwfx->wFormatTag);
		DebugOutput::trace("nChannels: %d", pwfx->nChannels);
		DebugOutput::trace("nSamplesPerSec: %d", pwfx->nSamplesPerSec);
		DebugOutput::trace("nAvgBytesPerSec: %d", pwfx->nAvgBytesPerSec);
		DebugOutput::trace("nBlockAlign: %d", pwfx->nBlockAlign);
		DebugOutput::trace("wBitsPerSample: %d", pwfx->wBitsPerSample);
		DebugOutput::trace("cbSize: %d", pwfx->cbSize);

		is_stoped = false;
		worker_ = new thread([&]() {
			while (is_stoped == false) do_repeat();
		});

		return true;
	}

	void close()
	{
		if (worker_ == nullptr) return;

		if (worker_ != nullptr) {
			is_stoped = true;
			worker_->join();
			worker_ = nullptr;
		}

		if (pAudioClient != nullptr) pAudioClient->Stop();

		fin_values();
	}

	int getChannels() { return pwfx->nChannels; }
	int getSamples() { return pwfx->nSamplesPerSec; }
	int getBitsPerSample() { return pwfx->wBitsPerSample; }

	float getVolume()
	{
		float volume = 0;
		if (SUCCEEDED(pAudioClient->GetService(IID_ISimpleAudioVolume, (void**) &pAudioVolume))) {
			pAudioVolume->GetMasterVolume(&volume);
		}
		return volume;
	}

	void setVolume(float value)
	{
		if (SUCCEEDED(pAudioClient->GetService(IID_ISimpleAudioVolume, (void**) &pAudioVolume))) {
			pAudioVolume->SetMasterVolume(value, NULL);
		}
	}

	void setOnData(DataEvent event) { on_data_ = event; };

private:
	thread* worker_ = nullptr;

	bool is_stoped = true;

	HRESULT hr;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	IAudioCaptureClient* pCaptureClient = nullptr;
	ISimpleAudioVolume* pAudioVolume = nullptr;
	WAVEFORMATEX* pwfx = nullptr;

	BYTE* pData;
	DWORD flags;

	DataEvent on_data_ = nullptr;

	void init_values()
	{
		pEnumerator = NULL;
		pDevice = NULL;
		pAudioClient = NULL;
		pCaptureClient = NULL;
		pAudioVolume = NULL;
		pwfx = NULL;
	}

	void fin_values()
	{
		if (pwfx != nullptr) CoTaskMemFree(pwfx);
		SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)
	}

	void do_repeat()
	{
		UINT32 packetLength = 0;
		hr = pCaptureClient->GetNextPacketSize(&packetLength);
		if (FAILED(hr)) {
		 	//DebugOutput::trace("GetNextPacketSize() error = %d , hr = 0x%08x ", GetLastError(), hr);
			return;
		}
		if (packetLength != 0) {
			hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
			if (FAILED(hr)) {
				//DebugOutput::trace("GetNextPacketSize() error = %d , hr = 0x%08x", GetLastError(), hr);
				return;
			}

			if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == false) {
				int data_size_ = ((int) packetLength) * pwfx->nChannels * pwfx->wBitsPerSample / 8;
				if (on_data_ != nullptr) on_data_(this, pData, data_size_);
			}
		
			pCaptureClient->ReleaseBuffer(numFramesAvailable);
		} else {
			if (on_data_ != nullptr) on_data_(this, nullptr, 0);
		}
	}
};
