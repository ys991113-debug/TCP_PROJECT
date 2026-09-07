# 🌐 TCP Remote Device Control

Raspberry Pi 4에 연결된 **LED, 조도 센서, 7-Segment, 부저**를 원격으로 제어하는 TCP 기반 IoT 시스템입니다.

Ubuntu의 CLI 클라이언트와 웹 브라우저에서 동시에 장치를 제어하고, 실시간 조도 그래프와 자동 조명 모드를 사용할 수 있습니다.

![C](https://img.shields.io/badge/C-00599C?style=flat-square&logo=c&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/Raspberry%20Pi%204-A22846?style=flat-square&logo=raspberrypi&logoColor=white)
![TCP/IP](https://img.shields.io/badge/TCP%2FIP-1833-2F80ED?style=flat-square)
![HTTP](https://img.shields.io/badge/HTTP-8080-00A98F?style=flat-square)
![Make](https://img.shields.io/badge/Build-GNU%20Make-6D00CC?style=flat-square&logo=gnu&logoColor=white)

---

## 📌 프로젝트 개요

| 항목 | 내용 |
|---|---|
| 타깃 보드 | Raspberry Pi 4 (aarch64) |
| 개발 언어 | C |
| 통신 | TCP Socket, HTTP, JSON |
| 동시성 | POSIX Thread, Mutex, Semaphore |
| 하드웨어 | LED, PCF8591 조도 센서, Common Anode 7-Segment, Buzzer |
| 빌드 | GNU Make, `aarch64-linux-gnu-gcc` 크로스 컴파일 |

### 주요 기능

- LED ON/OFF 및 `HIGH`·`MID`·`LOW` 3단계 밝기 제어
- PCF8591 I2C ADC를 이용한 조도값 측정
- 7-Segment 숫자 표시 및 0~9 카운트다운
- 카운트다운 종료 시 학교종 멜로디 재생
- 조도 임계값에 따라 LED를 제어하는 AUTO 모드
- CLI와 웹 대시보드의 동시 접속 및 제어
- 실시간 조도 그래프와 장치 제어 웹 UI
- 서버 데몬화, PID·로그 관리, SIGTERM 안전 종료

---

## 🏗️ 시스템 구조

```text
┌──────────────────── Ubuntu / PC ────────────────────┐
│                                                     │
│  CLI Client ───────────── TCP :1833 ────────────┐   │
│                                                 │   │
│  Web Browser ── HTTP :8080 ── Web Server ───────┤   │
└─────────────────────────────────────────────────┼───┘
                                                  │ JSON
┌────────────────── Raspberry Pi 4 ───────────────▼───┐
│                                                    │
│  Multi-thread TCP Server                           │
│      ├─ JSON Parser (jsmn)                         │
│      ├─ AUTO Thread / Countdown Thread             │
│      └─ Device Dispatcher (Mutex)                  │
│             │                                      │
│             ├─ dlopen → libdev_led.so              │
│             ├─ dlopen → libdev_light.so            │
│             ├─ dlopen → libdev_seg.so              │
│             └─ dlopen → libdev_buzzer.so           │
│                         │                          │
│                 wiringPi GPIO / I2C                │
└─────────────────────────┼──────────────────────────┘
                          ▼
              LED · CDS · 7-Segment · Buzzer
```

웹 서버는 브라우저의 HTTP 요청을 장치 제어용 JSON 메시지로 변환해 내부 TCP 서버에 전달합니다. CLI와 웹은 동일한 TCP 프로토콜을 사용하므로 동시에 접속해도 같은 장치 제어 흐름을 공유합니다.

---

## ⚡ 하드웨어 핀맵

> 소스는 `wiringPiSetup()`의 wiringPi 번호 체계를 사용합니다. BCM·Physical Pin은 표준 Raspberry Pi 40-pin 헤더 기준입니다.

| 장치 | 신호 | wiringPi | BCM GPIO | Physical Pin | 비고 |
|---|---:|---:|---:|---:|---|
| LED | PWM | 2 | 27 | 13 | Software PWM, 0~100 |
| Buzzer | Signal | 26 | 12 | 32 | GPIO 토글 방식 음계 출력 |
| PCF8591 | SDA | 8 | 2 | 3 | I2C-1, address `0x48` |
| PCF8591 | SCL | 9 | 3 | 5 | ADC channel 0 |
| 7-Segment | A | 27 | 16 | 36 | Common Anode |
| 7-Segment | B | 28 | 20 | 38 | Common Anode |
| 7-Segment | C | 29 | 21 | 40 | Common Anode |
| 7-Segment | D | 22 | 6 | 31 | Common Anode |
| 7-Segment | E | 21 | 5 | 29 | Common Anode |
| 7-Segment | F | 24 | 19 | 35 | Common Anode |
| 7-Segment | G | 23 | 13 | 33 | Common Anode |

Common Anode 7-Segment는 `LOW`일 때 점등되고 `HIGH`일 때 꺼집니다. 배선 전 Raspberry Pi의 전원을 끄고 LED·7-Segment에는 적절한 전류 제한 저항을 사용하세요.

---

## 🧰 빌드 환경

### 요구사항

| 구분 | 요구사항 |
|---|---|
| 호스트 | Ubuntu Linux 또는 WSL |
| 호스트 컴파일러 | GCC |
| ARM 컴파일러 | `aarch64-linux-gnu-gcc` |
| 배포 도구 | SSH, SCP |
| 타깃 | Raspberry Pi OS 64-bit |
| 타깃 라이브러리 | wiringPi, pthread, dl |

```bash
sudo apt update
sudo apt install build-essential gcc-aarch64-linux-gnu openssh-client -y
```

Raspberry Pi에서는 I2C 인터페이스를 활성화하고 wiringPi 런타임 라이브러리를 사용할 수 있어야 합니다. 크로스 컴파일 시 `-lwiringPi`를 찾지 못하면 다음 심볼릭 링크도 확인하세요.

```bash
ln -s libwiringPi.so.3.18 cross/lib/libwiringPi.so
```

---

## 🛠️ 빌드 방법

### 1. 저장소 받기

```bash
git clone https://github.com/ys991113-debug/TCP_PROJECT.git
cd TCP_PROJECT
```

### 2. 빌드만 실행

```bash
make cross client web
```

| 산출물 | 설명 |
|---|---|
| `cross/server` | Raspberry Pi용 TCP 서버 |
| `cross/webserver` | Raspberry Pi용 epoll HTTP 서버 |
| `cross/libdev_led.so` | LED 제어 플러그인 |
| `cross/libdev_light.so` | 조도 센서 플러그인 |
| `cross/libdev_seg.so` | 7-Segment 플러그인 |
| `cross/libdev_buzzer.so` | 부저 플러그인 |
| `client` | Ubuntu/WSL용 CLI 클라이언트 |
| `web/webserver` | 호스트용 웹 서버 빌드 결과 |

### 3. 클린 빌드

```bash
make clean
make cross client web
```

> 기본 `make` 타깃은 빌드뿐 아니라 **배포와 CLI 실행까지 연속 수행**합니다. 빌드만 필요하면 `make cross client web`을 사용하세요.

---

## 🚀 배포 및 실행

### 자동 배포

```bash
make deploy \
  PI_HOST=<RPI_IP> \
  PI_USER=<RPI_USER> \
  PI_DIR=/home/<RPI_USER>/project
```

이 명령은 Raspberry Pi의 `1833`, `8080` 포트를 사용 중인 기존 프로세스를 종료하고, 실행 파일·장치 플러그인·웹 파일을 전송한 뒤 서버를 시작합니다.

### 전체 파이프라인

빌드 → 배포 → CLI 실행을 한 번에 수행하려면 다음과 같이 실행합니다.

```bash
make \
  PI_HOST=<RPI_IP> \
  PI_USER=<RPI_USER> \
  PI_DIR=/home/<RPI_USER>/project
```

### CLI 클라이언트

```bash
make run PI_HOST=<RPI_IP>
```

또는 직접 실행합니다.

```bash
./client <RPI_IP>
```

### 웹 대시보드

```text
http://<RPI_IP>:8080/
```

서버가 시작되면 웹 서버도 함께 실행됩니다. 대시보드는 `/light`를 1초마다 호출해 최근 30개의 조도값을 그래프로 표시합니다.

### 서버 로그와 종료

```bash
# Raspberry Pi에서 로그 확인
tail -f /tmp/tcpserver.log

# 호스트에서 TCP 서버 원격 종료
make stop PI_HOST=<RPI_IP> PI_USER=<RPI_USER>
```

---

## 🔁 TCP JSON 프로토콜

TCP 서버는 `1833` 포트에서 JSON 요청을 받습니다.

### 요청 예시

```json
{
  "v": 1,
  "cmd": "LED",
  "args": {
    "state": "ON",
    "level": "HIGH"
  },
  "id": 1
}
```

### 성공 응답 예시

```json
{
  "ok": true,
  "cmd": "LED",
  "data": {
    "state": "ON",
    "bright": "HIGH"
  },
  "id": 1
}
```

### 지원 명령

| `cmd` | `args` | 설명 |
|---|---|---|
| `LED` | `state: ON`, `level: HIGH \| MID \| LOW` | LED 켜기 및 밝기 설정 |
| `LED` | `state: OFF` | LED 끄기 |
| `LIGHT` | `{}` | 조도값 조회 (`0~255`) |
| `SEG` | `value: 0~9` | 숫자 표시 |
| `BUZZER` | `state: ON \| OFF` | 학교종 멜로디 시작·정지 |
| `COUNTDOWN` | `value: 0~9` | 지정 숫자부터 0까지 카운트다운 |
| `AUTO` | `state: ON \| OFF` | 조도 기반 LED 자동 제어 |

### 오류 응답

| 오류 | 의미 |
|---|---|
| `BAD_JSON` | JSON 파싱 실패 |
| `UNKNOWN_CMD` | 등록되지 않은 명령 |
| `BAD_ARG` | 명령 인자 범위 또는 값 오류 |
| `BUSY` | 다른 카운트다운이 이미 실행 중 |

---

## 🌍 웹 API

모든 엔드포인트는 `GET` 요청을 사용합니다.

| 엔드포인트 | 설명 |
|---|---|
| `/` | 웹 대시보드 |
| `/light` | 조도값 조회 |
| `/led/on/high` | LED 최대 밝기 |
| `/led/on/mid` | LED 중간 밝기 |
| `/led/on/low` | LED 최저 밝기 |
| `/led/off` | LED 끄기 |
| `/buzzer/on` | 부저 멜로디 시작 |
| `/buzzer/off` | 부저 정지 |
| `/seg/<0-9>` | 숫자 표시 |
| `/countdown/<0-9>` | 카운트다운 시작 |
| `/auto/on` | AUTO 모드 시작 |
| `/auto/off` | AUTO 모드 정지 |

---

## 📁 디렉터리 구조

```text
TCP_PROJECT/
├── include/
│   ├── device.h              # 장치 플러그인 공통 인터페이스
│   └── jsmn.h                # 경량 JSON 파서
├── src/
│   ├── client/
│   │   └── client.c          # CLI TCP 클라이언트
│   ├── server/
│   │   ├── main.c            # 데몬, 소켓, 스레드, AUTO·카운트다운
│   │   ├── proto.c           # JSON 요청 파싱 및 응답 생성
│   │   └── loader.c          # dlopen/dlsym 장치 로더
│   └── devices/
│       ├── led.c             # LED 및 Software PWM
│       ├── light.c           # PCF8591 조도 센서
│       ├── seg.c             # 7-Segment 숫자 표시
│       └── buzzer.c          # 부저 멜로디 스레드
├── web/
│   ├── webserver.c           # epoll 기반 HTTP-TCP 브리지
│   └── index.html            # 장치 제어 대시보드
├── cross/
│   ├── include/              # 크로스 컴파일용 wiringPi 헤더
│   └── lib/                  # 크로스 컴파일용 wiringPi 라이브러리
├── Makefile                  # 빌드·배포·실행 자동화
├── 실행과정.txt              # 기능별 실행 및 검증 기록
└── README.md
```

---

## 🧠 핵심 설계

### 동적 장치 플러그인

각 장치는 `dev_name`, `dev_init`, `dev_handle`, `dev_cleanup` 공통 인터페이스를 구현한 공유 라이브러리입니다. 서버는 `dlopen`과 `dlsym`으로 플러그인을 불러오고 명령 이름에 맞는 장치로 요청을 전달합니다.

### 다중 클라이언트와 GPIO 동기화

TCP 연결마다 분리된 pthread가 요청을 처리합니다. `dispatch_mutex`가 장치 디스패치를 직렬화해 CLI와 웹의 동시 GPIO 접근 충돌을 방지하며, 순차 명령에는 마지막 명령이 반영되는 Last-Write-Wins 방식이 적용됩니다.

### 카운트다운 중복 방지

카운트다운은 별도 스레드에서 실행되고, 초기값 1의 semaphore로 동시에 하나만 허용합니다. 실행 중 다시 요청하면 `BUSY` 오류를 반환합니다.

### AUTO 모드

AUTO 모드는 1초마다 조도값을 읽습니다. 측정값이 임계값 `180`보다 크면 LED를 켜고, 작거나 같으면 끕니다. `auto_mutex`와 상태 플래그로 중복 스레드 생성을 막습니다.

### 데몬 수명주기

TCP 서버는 데몬으로 실행되며 PID를 `/tmp/tcpserver.pid`, 로그를 `/tmp/tcpserver.log`에 저장합니다. SIGTERM을 받으면 장치 정리 함수와 `dlclose`를 실행한 뒤 PID 파일을 제거합니다.

---

## ✅ 검증 시나리오

- CLI와 웹 클라이언트 동시 접속
- LED 밝기 단계 및 Last-Write-Wins 확인
- 잘못된 JSON·명령·인자 오류 응답 확인
- 카운트다운 중복 요청 시 `BUSY` 확인
- AUTO ON 중복 요청 시 스레드 하나만 유지
- Ctrl+C 기반 CLI 정상 종료와 TCP 서버 SIGTERM 정리

상세 실행 로그와 테스트 과정은 [`실행과정.txt`](./실행과정.txt)에서 확인할 수 있습니다.

---

## ⚠️ 알려진 제약과 보안 주의

- TCP와 HTTP에 인증·암호화가 적용되지 않았습니다. 공용 인터넷에 직접 노출하지 말고 신뢰할 수 있는 로컬 네트워크나 VPN에서 사용하세요.
- TCP 서버는 모든 인터페이스의 `1833` 포트에서 수신하지만, `web/webserver.c`의 HTTP 바인딩 주소와 TCP 브리지 주소는 현재 소스의 `RIP` 값으로 고정되어 있습니다. `PI_HOST`만 바꿔서는 웹 서버 주소가 바뀌지 않으므로 다른 Raspberry Pi에서 실행할 때 `RIP`도 수정해야 합니다.
- `make stop`은 PID 파일의 TCP 서버만 종료합니다. 분리 실행된 웹 서버가 남아 있다면 `8080` 포트 프로세스를 별도로 확인하세요.
- 웹 그래프는 CDN의 Chart.js를 사용하므로 인터넷 연결이 없으면 그래프 라이브러리가 로드되지 않을 수 있습니다.
- TCP 메시지는 단일 `recv()`에서 JSON 한 건을 받는 것을 전제로 하며 별도 길이 프레이밍은 없습니다.
- Raspberry Pi GPIO와 wiringPi에 의존하므로 일반 PC에서는 실제 장치 제어 기능을 실행할 수 없습니다.

---

## 🔧 문제 해결

| 증상 | 해결 방법 |
|---|---|
| `aarch64-linux-gnu-gcc: not found` | `sudo apt install gcc-aarch64-linux-gnu` |
| `cannot find -lwiringPi` | `cross/lib/libwiringPi.so` 링크와 라이브러리 경로 확인 |
| `bind: Address already in use` | Raspberry Pi에서 `sudo fuser -k 1833/tcp 8080/tcp` 실행 후 재시작 |
| GPIO 또는 I2C 접근 실패 | `sudo ./server`로 실행하고 I2C 활성화·배선을 확인 |
| 웹 페이지 접속 불가 | `RIP` 설정을 확인하고 `http://<RPI_IP>:8080/`으로 접속 |
| 웹 그래프가 표시되지 않음 | 인터넷 연결과 Chart.js CDN 접근 여부 확인 |

---

## 🙋 주요 구현 범위

- TCP 멀티스레드 서버와 JSON 명령 프로토콜
- `dlopen` 기반 장치 제어 플러그인 구조
- LED·조도 센서·7-Segment·부저 드라이버
- epoll 기반 HTTP 웹 서버와 실시간 대시보드
- Mutex·Semaphore 기반 하드웨어 동기화
- AUTO 모드, 카운트다운, 데몬 수명주기
- ARM 크로스 컴파일 및 SSH/SCP 배포 자동화

**작성자: 이윤상**

---

## 📄 라이선스

본 프로젝트는 임베디드 리눅스와 네트워크 프로그래밍 학습 및 포트폴리오 목적으로 제작되었습니다. 별도의 라이선스가 명시되어 있지 않으므로 재사용이나 배포가 필요한 경우 작성자에게 문의하세요.
