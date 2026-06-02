# devctl — 디바이스 제어 데몬

라즈베리파이 기반 디바이스 원격 제어 시스템. TCP 소켓 서버가 JSON 프로토콜로 명령을 수신하고, 동적 로드(.so)된 디바이스 드라이버를 통해 LED·버저·조도센서·7세그·카메라를 제어한다.

---

## 디렉토리 구조

```
project/
├── include/                    공용 헤더
│   ├── proto.h                 JSON 파서 인터페이스
│   ├── device.h                .so 공통 ABI (dev_init / handle / cleanup / name)
│   ├── state.h                 공유 상태 구조체
│   └── net.h                   소켓 유틸리티
│
├── src/
│   ├── server/                 데몬 서버
│   │   ├── main.c              데몬화 + 쓰레드 생성
│   │   ├── loader.c            dlopen 동적 로드
│   │   ├── net.c               accept + 라인버퍼
│   │   ├── proto.c             JSON parse / build
│   │   ├── dispatch.c          명령 디스패치
│   │   ├── state.c             공유 상태 관리
│   │   ├── sensor_thread.c     조도 센서 폴링
│   │   ├── display_thread.c    7세그 카운트다운
│   │   ├── buzzer_thread.c     버저 제어
│   │   └── camera_thread.c     카메라 스트림
│   │
│   ├── devices/                디바이스 드라이버 (.so 빌드)
│   │   ├── led.c               → libdev_led.so
│   │   ├── buzzer.c            → libdev_buzzer.so
│   │   ├── light.c             → libdev_light.so
│   │   ├── seg.c               → libdev_seg.so
│   │   └── cam.c               → libdev_cam.so
│   │
│   └── client/
│       └── client.c            CLI 클라이언트 (시그널 처리 포함)
│
├── web/                        추가 기능 — 웹 UI
│   ├── index.html
│   ├── app.js
│   └── bridge.py               WebSocket ↔ TCP 브릿지
│
├── scripts/
│   ├── devctl.service          systemd 유닛 파일
│   └── install.sh              설치 스크립트
│
├── docs/
│   ├── 개발문서.md
│   ├── API명세.md
│   ├── 실행과정.txt            script 명령으로 캡처한 실행 로그
│   └── wireshark_capture.png   추가 기능 네트워크 시연 자료
│
├── Makefile
└── README.md
```

---

## 아키텍처 개요

```
Client (CLI / Web)
       │  JSON over TCP
       ▼
  server/main.c  ──▶  dispatch.c  ──▶  loader.c (dlopen)
       │                                    │
       │                            libdev_*.so (디바이스 드라이버)
       │
  쓰레드 구조
  ├── sensor_thread     조도 센서 주기적 읽기
  ├── display_thread    7세그 카운트다운 표시
  ├── buzzer_thread     버저 패턴 출력
  └── camera_thread     카메라 스트림 처리
```

- **동적 로드**: 각 디바이스는 독립 `.so`로 빌드되어 런타임에 `dlopen`으로 적재된다. `device.h`에 정의된 공통 ABI(`dev_init` / `handle` / `cleanup` / `name`)를 구현하면 서버 재빌드 없이 드라이버를 교체할 수 있다.
- **프로토콜**: 클라이언트↔서버 간 통신은 줄바꿈 구분 JSON(`proto.c`)을 사용한다. 명세는 `docs/API명세.md` 참조.
- **공유 상태**: `state.h` / `state.c`가 쓰레드 간 공유 데이터를 관리한다.
- **웹 브릿지**: `web/bridge.py`가 브라우저 WebSocket을 서버 TCP 소켓으로 중계한다.

---

## 빌드 및 실행

```bash
make                # 서버 + 클라이언트 + 모든 .so 빌드
make install        # scripts/install.sh 실행 (systemd 등록 포함)
```

서비스 직접 실행:

```bash
sudo systemctl start devctl
sudo systemctl status devctl
```

CLI 클라이언트:

```bash
./client <host> <port>
```

---

## 문서

| 파일 | 내용 |
|------|------|
| `docs/개발문서.md` | 설계 배경 및 구현 상세 |
| `docs/API명세.md` | JSON 프로토콜 명령 목록 |
| `docs/실행과정.txt` | 빌드~실행 전 과정 캡처 로그 |
| `docs/wireshark_capture.png` | 웹 브릿지 네트워크 패킷 시연 |
