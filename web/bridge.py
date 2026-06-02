import asyncio
import websockets
import socket
import json

RASP_IP   = "127.0.0.1"
RASP_PORT = 1833

async def handler(websocket):
    print("브라우저 연결됨")
    # 라파 TCP 서버에 연결
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((RASP_IP, RASP_PORT))
    sock.setblocking(False)

    req_id = 1

    async def send_light():
        nonlocal req_id
        req = json.dumps({"v":1,"cmd":"LIGHT","args":{},"id":req_id})
        req_id += 1
        sock.sendall((req + "\n").encode())

    try:
        while True:
            # 1초마다 조도값 요청
            await send_light()
            await asyncio.sleep(1)

            # TCP 응답 읽기
            loop = asyncio.get_event_loop()
            data = await loop.run_in_executor(None, sock.recv, 4096)
            if not data:
                break

            # 브라우저로 전송
            await websocket.send(data.decode().strip())

    except Exception as e:
        print(f"오류: {e}")
    finally:
        sock.close()

async def main():
    async with websockets.serve(handler, "0.0.0.0", 8000):
        print("WebSocket 서버 시작 → 포트 8000")
        await asyncio.Future()

asyncio.run(main())