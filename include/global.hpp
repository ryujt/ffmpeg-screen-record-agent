#pragma once

#include <windows.h>

const unsigned int WM_RECORD_AGENT_STOP = WM_USER + 1;
const unsigned int WM_RECORD_AGENT_PING = WM_USER + 2;
const unsigned int WM_RECORD_AGENT_ERROR = WM_USER + 3;
const unsigned int WM_RECORD_AGENT_PAUSE = WM_USER + 4;
const unsigned int WM_RECORD_AGENT_FRAMES_IN_BUFFER = WM_USER + 5;
const unsigned int WM_RECORD_AGENT_COMPLETED = WM_USER + 6;
const unsigned int WM_RECORD_AGENT_TERMINATE = WM_USER + 7;
const unsigned int WM_RECORD_AGENT_GET_STATUS = WM_USER + 8;

/*-------------------------------------------------------------------------------------------------*/

// 처리되지 않은 unknown exception이 발생한 경우
const static int ERROR_CREATE_UNKNOWN = -999;

// 인코더를 생성하는 데 실패 (시작 옵션의 문제가 있는 경우)
const static int ERROR_CREATE_ENCODER = -1;

// 오디오 장치를 여는 데 실패 (녹화 시작 이후에 pause, resume 등에서 발생)
const static int ERROR_AUDIO_OPEN = -2;

// 컴퓨터의 디스크 용량이 부족하여 에러가 발생
const static int ERROR_INSUFFICIENT_DISK = -3;

// 컴퓨터의 메모리가 부족하여 에러가 발생
const static int ERROR_INSUFFICIENT_MEMORY = -4;

// 녹화 시작 옵션으로 전달되는 JSON 파싱 중에 에러가 발생
const static int ERROR_PARSING_JSON = -5;

// 인코딩 중에 오류가 발생
const static int ERROR_ENCODING = -6;

// JPEG 저장 중 오류 발생
// 버퍼에 화면데이터가 일정 이상 쌓이면 디스크에 JPEG으로 저장한다.
const static int ERROR_JPEG_CREATE = -7;

// JPEG 읽기 중 오류 발생
const static int ERROR_JPEG_READ = -8;

// 메인 프로그램으로부터 ping 메시지가 일정 시간 이상 들어오지 않았다.
const static int ERROR_PING_TIMEOUT = -9;

// 녹화가 시작된 상태에서 다시 시작을 요청했다.
const static int ERROR_RECODER_IS_STARTED = -10;

// 파일 저장 중에 에러가 발생
const static int ERROR_IN_WRITTING_STREAM_FILE = -11;

// 녹화 시작 중에 오디오 장치를 오픈한데 실패했다.
const static int ERROR_START_AUDIO = -12;

// 녹화 시작 중에 파일을 생성(오픈)하는데 실패했다.
const static int ERROR_CREATE_VIDEO_FILE = -13;

/*-------------------------------------------------------------------------------------------------*/

const static int BITMAP_PIXEL_SIZE = 4;
const static int BITMAP_CELL_SIZE = 8;

const static int AUDIO_PACKET_SIZE = 4096;
const static int AUDIO_CHANNEL = 1;
const static int AUDIO_SAMPLE_RATE = 44100;
const static int AUDIO_SAMPLE_SIZE = 4;
const static int AUDIO_FRAMES_PER_BUFFER = 1024;
const static int AUDIO_FRAMES_SIZE = AUDIO_FRAMES_PER_BUFFER * AUDIO_CHANNEL * AUDIO_SAMPLE_SIZE;
