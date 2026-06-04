  ## 0. 한눈에 보는 구조 (어디서 무엇을 실행하나)

  | 머신 | 역할 | 실행하는 것 |
  |------|------|------------|
  | 우분투 | 개발 / 빌드 / 클라이언트 | make(빌드), make deploy(전송), make run(CLI) |
  | 라즈베리파이 4 | 서버 (하드웨어 연결) | TCP 서버 + 웹서버 (deploy가 자동 실행) |

  - 라파에서 `sudo ./server` 하나로 TCP 서버 + 웹서버가 동시에 실행됩니다.
  - 빌드는 우분투에서 크로스 컴파일(ARM)로 이루어집니다.

  ---

  ## 1. 사전 준비

  ### 1-1. 우분투 (빌드용) — 필수 패키지 설치

      sudo apt update
      sudo apt install gcc make git gcc-aarch64-linux-gnu -y

  > gcc-aarch64-linux-gnu 가 크로스 컴파일러입니다. 이게 없으면 빌드되지 않습니다.

  ### 1-2. 라즈베리파이 — 확인 사항

  - wiringPi 라이브러리 설치 (ls /usr/lib/libwiringPi.so* 로 확인)
  - I2C 활성화 (조도센서 PCF8591용)
  - 우분투에서 SSH 접속 가능 (ssh <계정>@<라파IP>)

  ### 1-3. 네트워크

  - 우분투와 라즈베리파이가 Tailscale(또는 같은 네트워크)로 연결
  - 확인: 우분투에서 ping <라파IP> 응답하면 OK

  ---

  ## 2. 빌드 (우분투에서)

      git clone https://github.com/ys9991113-lgtm/TCP_PROJECT.git
      cd TCP_PROJECT
      make

  | 산출물 | 아키텍처 | 설명 |
  |--------|----------|------|
  | cross/server | ARM | 라파용 TCP 서버 (데몬) |
  | cross/libdev_*.so (4개) | ARM | 라파용 장치 라이브러리 |
  | cross/webserver | ARM | 라파용 웹서버 (epoll) |
  | client | x86 | 우분투용 CLI 클라이언트 |

  확인: `file cross/server` → ARM aarch64 가 나오면 성공.

  ---

  ## 3. 라파로 배포 + 서버 실행

  ### 3-1. 변수 설명 (중요)

  | 변수 | 의미 | 기본값 |
  |------|------|--------|
  | PI_HOST | 라파 IP | 100.83.88.7 |
  | PI_USER | 라파 계정 | lys |
  | PI_DIR | 라파 프로젝트 경로 | /home/lys/project |

  라파에서 확인: `whoami` (계정), `echo $HOME` (경로)

  ### 3-2. 배포 명령 (우분투에서)

      make deploy PI_HOST=<라파IP> PI_USER=<라파계정> PI_DIR=/home/<라파계정>/project

  예시:

      make deploy PI_HOST=100.96.92.89 PI_USER=pi PI_DIR=/home/pi/project

  자동 수행: ① 기존 서버 종료 ② 폴더 생성 ③ 파일 전송 ④ 서버+웹서버 실행

  > ⚠️ 마지막 단계에서 라파 sudo 비밀번호 오류(`sudo: a terminal is required`)로
  > 멈출 수 있습니다. 이 경우 파일 전송은 이미 완료된 상태이므로,
  > 아래 3-3 방법으로 라파에서 직접 서버를 실행하세요.

  ### 3-3. (비번에서 막힐 경우) 라파에서 직접 실행

      cd ~/project
      sudo fuser -k 1833/tcp 8080/tcp 2>/dev/null
      sudo ./server

  성공 확인:

      cat /tmp/tcpserver.log

  아래가 보이면 정상:

      [daemon] 시작 PID=...
      [loader] LED 로드 완료
      [loader] LIGHT 로드 완료
      [loader] SEG 로드 완료
      [loader] BUZZER 로드 완료
      서버 시작 → 포트 1833

  ---

  ## 4. 실행 / 동작 확인

  ### 4-1. 웹 브라우저 (조도 모니터링 + 장치 제어)

      http://<라파IP>:8080/

  예) http://100.83.88.7:8080/

  - Tailscale 연결된 PC·모바일 어디서든 접속 가능
  - 조도 그래프(1초 갱신) + LED/부저/카운트다운/7세그/AUTO 버튼

  ### 4-2. CLI 클라이언트 (우분투)

      make run PI_HOST=<라파IP>

  메뉴:

      1) LED ON(최대)    2) LED ON(중간)
      3) LED ON(최저)    4) LED OFF
      5) 조도 읽기
      6) 7세그 표시(0~9)
      7) 부저 ON         8) 부저 OFF
      9) 카운트다운(0~9)   → 0 도달 시 부저 자동 울림
      10) AUTO ON        11) AUTO OFF
      0) 종료

  시그널 처리:

  | 키 | 동작 |
  |----|------|
  | Ctrl+C | 정상 종료 |
  | Ctrl+Z, Ctrl+\ | 무시 (강제 종료 불가) |

  ---

  ## 5. 서버 종료

      make stop PI_HOST=<라파IP> PI_USER=<라파계정>

  또는 라파에서 직접:

      sudo kill $(cat /tmp/tcpserver.pid)

  ---

  ## 6. Makefile 명령 정리

  | 명령 | 동작 | 실행 위치 |
  |------|------|----------|
  | make | 전체 빌드 (크로스 컴파일) | 우분투 |
  | make deploy | 라파 전송 + 서버 자동 실행 | 우분투 |
  | make run | CLI 클라이언트 실행 | 우분투 |
  | make stop | 라파 서버 종료 | 우분투 |
  | make clean | 빌드 산출물 삭제 | 우분투 |

  ---

  ## 7. 구현 기능

  | 장치 | 기능 |
  |------|------|
  | LED | ON/OFF, 밝기 3단계 (최대/중간/최저, softPWM) |
  | 부저 | 학교종 멜로디 ON/OFF |
  | 조도센서 | PCF8591 I2C, 실시간 값 읽기 (웹 그래프) |
  | 7세그먼트 | 0~9 표시 / 카운트다운(1초 감소) + 0 도달 시 부저 |

  부가 기능: 웹 대시보드, AUTO 모드(조도→LED), 다중 클라이언트(CLI+웹 동시 접속)

  ---

  ## 8. 주요 기술

  - TCP 멀티스레드 서버 / JSON 프로토콜 (jsmn)
  - 동적 라이브러리(.so) 플러그인 구조 (dlopen) — .so만 교체해 기능 업그레이드
  - 데몬 프로세스 + 시그널 처리 (SIGTERM 안전 종료)
  - 크로스 컴파일 (우분투 x86 → 라파 ARM aarch64)
  - 동기화 (mutex: GPIO/AUTO 보호, semaphore: 카운트다운 중복 방지)
  - epoll 기반 HTTP 웹서버

  ---

  ## 9. 프로젝트 구조

      TCP_PROJECT/
      ├── src/
      │   ├── server/      # main.c, proto.c, loader.c
      │   ├── devices/     # led, light, seg, buzzer (.so 소스)
      │   └── client/      # client.c
      ├── web/             # webserver.c + index.html
      ├── cross/           # 크로스컴파일용 wiringPi (헤더/라이브러리)
      ├── include/         # device.h, jsmn.h
      ├── 실행과정.txt
      ├── README.md
      └── Makefile

  ---

  ## 10. 문제 해결 (Troubleshooting)

  | 증상 | 원인 / 해결 |
  |------|------------|
  | wiringPi.h: No such file | 크로스 헤더 누락 → cross/include/ 가 있는지 확인 |
  | aarch64-linux-gnu-gcc: not found | 크로스 툴체인 미설치 → sudo apt install gcc-aarch64-linux-gnu |
  | sudo: a terminal is required (deploy 중) | 정상. 전송은 완료됨 → 3-3으로 라파에서 직접 sudo ./server |
  | bind: Address already in use | 포트 점유 → sudo fuser -k 1833/tcp 8080/tcp 후 재실행 |
  | scp ... Failure | 라파 서버 실행 중이라 덮어쓰기 실패 → 라파에서 sudo fuser -k 1833/tcp 후 재배포 |
  | 웹 localhost:8080 접속 안 됨 | 웹서버는 라파에서 실행됨 → http://<라파IP>:8080/ 으로 접속 |

  ---

  ## 11. 통신 포트

  | 포트 | 용도 |
  |------|------|
  | TCP 1833 | 장치 제어 (JSON 프로토콜) |
  | HTTP 8080 | 웹 모니터링 / 제어 |