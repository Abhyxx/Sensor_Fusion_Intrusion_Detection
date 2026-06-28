import argparse
import csv
import os
import time
from datetime import datetime

import websocket


def safe_float(value):
    try:
        return float(value)
    except Exception:
        return -1.0


def safe_int(value):
    try:
        return int(value)
    except Exception:
        return -1


def main():
    parser = argparse.ArgumentParser(
        description="Collect ESP32 IDS dataset over WebSocket on Windows. PIR removed version."
    )
    parser.add_argument("--ip", required=True, help="ESP32 IP address shown in Arduino Serial Monitor")
    parser.add_argument("--label", required=True, help="Ground-truth label for this experiment")
    parser.add_argument("--duration", type=int, default=60, help="Recording duration in seconds")
    parser.add_argument("--trial", default="trial_01", help="Trial ID, for example trial_01")
    parser.add_argument("--out", default="raw", help="Output folder. Default: raw")

    args = parser.parse_args()
    os.makedirs(args.out, exist_ok=True)

    file_time = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = os.path.join(args.out, f"{args.label}_{args.trial}_{file_time}.csv")

    ws_url = f"ws://{args.ip}:81"
    print(f"Connecting to ESP32 WebSocket: {ws_url}")
    print(f"Saving to: {filename}")
    print(f"Label: {args.label}")
    print("Press Ctrl+C to stop early.\n")

    ws = websocket.WebSocket()
    ws.connect(ws_url, timeout=5)

    header = [
        "timestamp_iso",
        "unix_ms",
        "trial_id",
        "label",
        "radar_status",
        "pir_placeholder",
        "distance_cm",
        "inference",
        "speed_m_s",
        "esp_millis",
        "radar_bin",
        "ultrasonic_active",
        "inference_code",
        "distance_change_cm",
        "speed_cm_s",
        "radar_count",
        "raw_message",
    ]

    start_time = time.time()
    row_count = 0

    try:
        with open(filename, "w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(header)
            file.flush()

            while True:
                if time.time() - start_time >= args.duration:
                    break

                message = ws.recv().strip()
                parts = [p.strip() for p in message.split(",")]

                if len(parts) < 12:
                    print("Skipping incomplete row:", message)
                    continue

                radar_status = parts[0]
                pir_placeholder = parts[1]
                distance_cm = safe_float(parts[2])
                inference = parts[3]
                speed_m_s = safe_float(parts[4])
                esp_millis = safe_int(parts[5])
                radar_bin = safe_int(parts[6])
                ultrasonic_active = safe_int(parts[7])
                inference_code = safe_int(parts[8])
                distance_change_cm = safe_float(parts[9])
                speed_cm_s = safe_float(parts[10])
                radar_count = safe_int(parts[11])

                row = [
                    datetime.now().isoformat(timespec="milliseconds"),
                    int(time.time() * 1000),
                    args.trial,
                    args.label,
                    radar_status,
                    pir_placeholder,
                    distance_cm,
                    inference,
                    speed_m_s,
                    esp_millis,
                    radar_bin,
                    ultrasonic_active,
                    inference_code,
                    distance_change_cm,
                    speed_cm_s,
                    radar_count,
                    message,
                ]

                writer.writerow(row)
                file.flush()
                row_count += 1
                print(row)

    except KeyboardInterrupt:
        print("\nRecording stopped manually.")
    finally:
        ws.close()
        print(f"\nSaved {row_count} rows to {filename}")


if __name__ == "__main__":
    main()
