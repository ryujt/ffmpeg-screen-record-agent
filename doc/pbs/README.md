# PBS (Process Breakdown Structure)

1. **Recorder Agent**
   1. 녹화 시작/종료 (녹화 옵션 정보 포함)
   2. 메인프로그램 동작 감지 (메인프로그램 응답 없으면 녹화 종료)
   3. 녹화 진행 상황 전송 (오류)
2. **Audio Module**
   1. 오디오 입력 장치 목록 보기
   2. 오디오 장치 선택 (최대 2개 선택: 마이크, 스테레오 믹스)
   3. 오디오 입력 볼륨 (get/set)
3. **Desktop Capture**
   1. 캡쳐 영역 설정 (get/set)
   2. 초당 프레임 수 (get/set)
   3. 마우스 커서 캡쳐 여부 (get/set)
4. **Video Creater**
   1. 녹화 파일명 (경로포함, get/set)
   2. 압축 속도(품질, get/set)

::: tip
Recorder Agent의 경우에는 메인프로그램과 윈도우 메시지 또는 파이프 등으로 교신하여 녹화를 진행합니다.
:::
