# Record Agent Message Protocol


## 녹화시작(Start) 메시지 (Main --> Agent)

메인 프로그램과 녹화 에이전트는 윈도우 메시지를 이용해서 통신합니다.
전달해야할 정보가 복잡하여 WM_COPYDATA를 이용합니다.

``` cpp
    std::string json = "{...}";
    COPYDATASTRUCT data;
    data.dwData = 0;
    data.cbData = json.length();
    data.lpData = (void*) json.c_str();
    int result = SendMessage(
        수신 윈도우 핸들,
        WM_COPYDATA,
        송신 윈도우 핸들,
        (LPARAM) &data);
```
* result
  * 0: 오류 없음

에러는 아래의 코드 중 하나가 리턴됩니다.

``` cpp
// 인코더를 생성하는 데 실패 (시작 옵션의 문제가 있는 경우)
const static int ERROR_CREATE_ENCODER = -1;

// 녹화가 시작된 상태에서 다시 시작을 요청했다.
const static int ERROR_RECODER_IS_STARTED = -10;

// 녹화 시작 중에 오디오 장치를 오픈한데 실패했다.
const static int ERROR_START_AUDIO = -12;

// 녹화 시작 중에 파일을 생성(오픈)하는데 실패했다.
const static int ERROR_CREATE_VIDEO_FILE = -13;
```


### 캡쳐할 영역을 지정하는 방식

``` json
{
    "code": "start",
    "filename": "C:/Work/test01.mp4",
    "mic": -1,
    "system-audio": true,
    "volume-mic": 1.0,
    "volume-system": 1.0,
    "screen-type": "region",
    "fps": 25,
    "speed": "ultrafast",
    "bitrate-KB": -1,
    "gop-size": 16,
    "use-nvenc": false,
    "left": 0,
    "top": 0,
    "width": 1920,
    "height": 1080,
    "with-cursor": true,
    "timeout": -1
}
```
* "mic"
  * -1: 오디오 입력 장치 중에 기본으로 설정되어 있는 장치를 선택합니다.
  * 999: 채널을 사용하지 않겠다는 의미입니다.
  * 기타 숫자: 오디오 장지 아이디, [AudioDevice.dll 사용법](/manuals/AudioDevice)을 참고하세요
* "system-audio"
  * 시스템 사운드 캡쳐 여부
  * true: 시스템 사운드 캡쳐 사용, false: 시스템 사운드 캡쳐 사용하지 않음
* volume-mic, volume-system
  * 오디오 서로 섞을 때 볼륨을 조절하여 섞도록 합니다.
* fps
  * 초당 프레임 수
* speed
  * x264 인코딩 속도
  * 하드웨어 인코딩을 하는 경우에는 적용되지 않습니다.
  * ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow
* bitrate-KB
  * x264 인코딩 bitrate
  * -1은 디폴트 설정으로 적용됩니다.
  * 단위가 KB인것에 유의하세요.
* use-nvenc
  * NVIDIA를 이용한 하드웨어 가속기 사용 여부
* left, top
  * 화면을 캡쳐할 영역의 좌상단의 좌표
* width, height
  * 화면을 캡쳐할 영역의 크기
  * left, top에 영향받지 않는 독립적인 화면의 넓이 높이입니다.
* with-cursor
  * 커서를 함께 녹화할 것인지에 대한 여부
* timeout
  * 메인 프로그램에서 ping 메시지가 들어오지 않으면 녹화를 중단하고 녹화 에이전트를 종료합니다.
  * 단위는 초
  * -1이면 timeout을 처리하지 않습니다.


### 전체화면을 캡쳐하는 경우

``` json
{
    "code": "start",
    "filename": "C:/Work/test01.mp4",
    "mic": -1,
    "system-audio": true,
    "volume-mic": 1.0,
    "volume-system": 1.0,
    "screen-type": "full",
    "fps": 25,
    "with-cursor": true
}
```


### 선택된 윈도우 화면을 캡쳐하는 경우

``` json
{
    "code": "start",
    "filename": "C:/Work/test01.mp4",
    "mic": -1,
    "system-audio": true,
    "volume-mic": 1.0,
    "volume-system": 1.0,
    "screen-type": "window",
    "fps": 25,
    "window-handle": 1234,
    "with-cursor": true
}
```
* window-handle
  * 화면을 캡쳐할 윈도의 핸들 값 (HWND)


## 녹화중지(Stop) 메시지 (Main --> Agent)

녹화중지 메시지의 경우에는 메시지 종류만 구별하면 되기 때문에 PostMessage를 사용합니다.
SendMessage를 사용하셔도 무방하지만,
리턴이 되는 것을 기다리면서 혹시나 병목이나 데드락을 발생시킬 수도 있어서 PostMessage를 권장합니다.

``` cpp
const int WM_RECORD_AGENT_STOP = WM_USER + 1;
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_STOP, 0, 0);
```


## Ping 메시지 (Main --> Agent)

메인 프로그램에서 주기적으로 발생시켜서 메인 프로그램이 제대로 동작하는 지 여부를 알립니다.
1초 이상의 간격으로 보내주시면 5초 이상 메시지가 들어오지 않으면 현재의 녹화를 중단하도록 하겠습니다.

``` cpp
const int WM_RECORD_AGENT_PING = WM_USER + 2;
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_PING, 0, 0);
```

::: tip
최초 설계시에는 메인 프로그램의 윈도우 핸들을 감시하는 것으로 진행했으나, 메인 프로그램이 응답 없음에도 핸들은 찾을 수 있기 때문에 메인 프로그램에서 신호를 주기적으로 보내는 것이 더욱 안전한 해결방법이라고 생각합니다.
:::


## 에러 메시지 (Agent --> Main)

``` cpp
const int WM_RECORD_AGENT_ERROR = WM_USER + 3;
int error_code = 0;
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_ERROR, error_code, 0);

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
```


## 일시 멈춤/재시작 메시지 (Main --> Agent)

재시작을 Start 메시지를 이용할 수도 있으나 파라메터가 중복해서 올 필요가 없고, 
WM_COPYDATA는 PostMessage를 사용할 수가 없어서 병목 등을 유발할 수도 있어서 
wParam을 이용해서 멈춤/재시작을 구별하도록 했습니다.

멈춤/재시작의 경우에는 같은 파일에 계속 이어서 녹화가 진행됩니다.

``` cpp
const int WM_RECORD_AGENT_PAUSE = WM_USER + 4;

// 멈춤
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_PAUSE, 1, 0);

// 재시작
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_PAUSE, 0, 0);
```
* wParam
  * 1: 일시 멈춤
  * 0: 재시작


## 인코딩 진행 상황 메시지 (Agent --> Main)

``` cpp
const int WM_RECORD_AGENT_FRAMES_IN_BUFFER = WM_USER + 5;
int frame_count;
int estimated_end_time;
PostMessage(
    수신 윈도우 핸들, 
    WM_RECORD_AGENT_FRAMES_IN_BUFFER, 
    frame_count, 
    estimated_end_time);
```
* wParam
  * 인코더의 버퍼에 남은 프레임 개수
  * 0이 전송되면 인코딩이 완료 된 것으로 처리합니다.
    * 0이 된 시점은 한 번만 메시지가 전송됩니다.
  * 전송 주기는 fps 단위 마다 보내도록 하겠습니다.
    * 컴퓨터 성능에 따라 메시지 전송 속도도 자동으로 조절되고 간편한 방법으로 생각됩니다.
* lParam
  * 인코딩 종료가 남은 시간 (단위 초)
  * 이전에 메시지 보낸 시간과 현재 메시지의 전송 시간의 차이로 남은 프레임 수를 나눠서 계산


## 인코딩 종료 메시지 (Agent --> Main)

``` cpp
const int WM_RECORD_AGENT_COMPLETED = WM_USER + 6;
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_COMPLETED, 0, 0);
```
* 인코딩이 완전히 마무리되고 더이상 인코딩 할 데이터가 없다는 것을 알립니다.


## 녹화 에이전트 프로그램 종료 메시지 (Main --> Agent)

``` cpp
const int WM_RECORD_AGENT_TERMINATE = WM_USER + 7;
PostMessage(수신 윈도우 핸들, WM_RECORD_AGENT_TERMINATE, 0, 0);
```
* 녹화 에이전트 프로그램을 종료시킵니다.


## 녹화 에이전트 상태가져오기 (Main --> Agent)

``` cpp
const int WM_RECORD_AGENT_GET_STATUS = WM_USER + 8;
int result = SendMessage(수신 윈도우 핸들, WM_RECORD_AGENT_GET_STATUS, 0, 0);
```
* result
  * 0: 인코딩 중 (stop 신호를 받았어도 큐에 남은 것이 있으면 큐가 빌 때까지 0으로 리턴됩니다.)
  * 1: 정지
  * 2: 일시 정지