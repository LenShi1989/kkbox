#!/usr/bin/env python3
"""KKBOX 序列埠遠端測試工具。

用法:
    python kkbox_monitor.py --port COM3            # 互動模式
    python kkbox_monitor.py --port COM3 --led blink
    python kkbox_monitor.py --port COM3 --raw '{"cmd":"status"}'

互動模式下可輸入:
    led on | led off | led blink    遠端控制板載 LED1
    arm | disarm                    解鎖 / 上鎖
    cal                             陀螺儀零點校正
    thr <1000-2000>                 設定油門 (us)
    sp <roll> <pitch> <yaw>         設定角速率目標 (deg/s)
    pid <roll|pitch|yaw> <kp> <ki> <kd>
    tel <ms>                        遙測週期 (0=關閉)
    status | ping
    raw <json>
    q / quit                        離開

需要 pyserial: pip install pyserial
"""

import argparse
import json
import sys
import threading
import time

try:
    import serial
except ImportError:
    sys.exit("缺少 pyserial，請先執行: pip install pyserial")


def reader(ser, stop):
    while not stop.is_set():
        try:
            raw = ser.readline()
        except serial.SerialException:
            break
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").strip()
        if not line:
            continue
        try:
            obj = json.loads(line)
            print("\n<-", json.dumps(obj, ensure_ascii=False))
        except json.JSONDecodeError:
            print("\n<-", line)


def send(ser, payload):
    data = json.dumps(payload, separators=(",", ":")) + "\n"
    ser.write(data.encode("utf-8"))
    print("->", data.strip())


def command_to_json(parts):
    op = parts[0].lower()

    if op == "led":
        if len(parts) == 2 and parts[1] in ("on", "off", "blink"):
            return {"cmd": "led", "mode": parts[1]}
        return {"cmd": "led", "value": int(parts[1]) if len(parts) > 1 else 0}
    if op == "arm":
        return {"cmd": "arm"}
    if op == "disarm":
        return {"cmd": "disarm"}
    if op == "cal":
        return {"cmd": "calibrate"}
    if op == "thr":
        return {"cmd": "throttle", "value": int(parts[1])}
    if op == "sp":
        return {"cmd": "setpoints",
                "roll": float(parts[1]), "pitch": float(parts[2]), "yaw": float(parts[3])}
    if op == "pid":
        return {"cmd": "pid", "axis": parts[1],
                "kp": float(parts[2]), "ki": float(parts[3]), "kd": float(parts[4])}
    if op == "tel":
        return {"cmd": "telemetry", "value": int(parts[1])}
    if op in ("status", "ping"):
        return {"cmd": op}
    if op == "raw":
        return json.loads(" ".join(parts[1:]))
    return None


def main():
    ap = argparse.ArgumentParser(description="KKBOX serial monitor")
    ap.add_argument("--port", required=True, help="序列埠, 例如 COM3 或 /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=9600)
    ap.add_argument("--led", choices=["on", "off", "blink"], help="單次送出 LED 命令後結束")
    ap.add_argument("--raw", help="送出原始 JSON 命令後結束")
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)

    if args.led:
        send(ser, {"cmd": "led", "mode": args.led})
        time.sleep(0.3)
        print(ser.read_all().decode("utf-8", "replace"))
        ser.close()
        return

    if args.raw:
        ser.write((args.raw + "\n").encode("utf-8"))
        time.sleep(0.5)
        print(ser.read_all().decode("utf-8", "replace"))
        ser.close()
        return

    send(ser, {"cmd": "telemetry", "value": 50})
    stop = threading.Event()
    t = threading.Thread(target=reader, args=(ser, stop), daemon=True)
    t.start()

    print("已連線。輸入 help 或參考檔頭說明, q 離開。")
    try:
        while True:
            try:
                line = input("> ").strip()
            except EOFError:
                break
            if not line:
                continue
            if line in ("q", "quit", "exit"):
                break
            if line == "help":
                print(__doc__)
                continue
            parts = line.split()
            payload = command_to_json(parts)
            if payload is None:
                print("未知命令:", line)
                continue
            send(ser, payload)
    finally:
        stop.set()
        ser.close()


if __name__ == "__main__":
    main()
