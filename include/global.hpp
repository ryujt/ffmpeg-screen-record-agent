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

// ó������ ���� unknown exception�� �߻��� ���
const static int ERROR_CREATE_UNKNOWN = -999;

// ���ڴ��� �����ϴ� �� ���� (���� �ɼ��� ������ �ִ� ���)
const static int ERROR_CREATE_ENCODER = -1;

// ����� ��ġ�� ���� �� ���� (��ȭ ���� ���Ŀ� pause, resume ��� �߻�)
const static int ERROR_AUDIO_OPEN = -2;

// ��ǻ���� ��ũ �뷮�� �����Ͽ� ������ �߻�
const static int ERROR_INSUFFICIENT_DISK = -3;

// ��ǻ���� �޸𸮰� �����Ͽ� ������ �߻�
const static int ERROR_INSUFFICIENT_MEMORY = -4;

// ��ȭ ���� �ɼ����� ���޵Ǵ� JSON �Ľ� �߿� ������ �߻�
const static int ERROR_PARSING_JSON = -5;

// ���ڵ� �߿� ������ �߻�
const static int ERROR_ENCODING = -6;

// JPEG ���� �� ���� �߻�
// ���ۿ� ȭ�鵥���Ͱ� ���� �̻� ���̸� ��ũ�� JPEG���� �����Ѵ�.
const static int ERROR_JPEG_CREATE = -7;

// JPEG �б� �� ���� �߻�
const static int ERROR_JPEG_READ = -8;

// ���� ���α׷����κ��� ping �޽����� ���� �ð� �̻� ������ �ʾҴ�.
const static int ERROR_PING_TIMEOUT = -9;

// ��ȭ�� ���۵� ���¿��� �ٽ� ������ ��û�ߴ�.
const static int ERROR_RECODER_IS_STARTED = -10;

// ���� ���� �߿� ������ �߻�
const static int ERROR_IN_WRITTING_STREAM_FILE = -11;

// ��ȭ ���� �߿� ����� ��ġ�� �����ѵ� �����ߴ�.
const static int ERROR_START_AUDIO = -12;

// ��ȭ ���� �߿� ������ ����(����)�ϴµ� �����ߴ�.
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
