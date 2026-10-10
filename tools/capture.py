"""Receive the telemetry stream on the PC, print alerts, and log raw data to CSV.

Use it two ways:
  1. As a data logger (phase 1: build your real dataset through the real link).
  2. As a reference receiver to compare against the Nucleo's STATUSTEXT alerts.

    python tools/capture.py --port 14550 --csv data/run1.csv
Columns: arrival_s, sensor_ms, value, seq, sysid   (train on sensor_ms, never arrival_s)
"""
import argparse, csv, pathlib, time

from pymavlink import mavutil


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=14550)
    ap.add_argument("--csv", type=pathlib.Path)
    args = ap.parse_args()

    link = mavutil.mavlink_connection(f"udpin:0.0.0.0:{args.port}")
    out = None
    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        out = csv.writer(args.csv.open("w", newline=""))
        out.writerow(["arrival_s", "sensor_ms", "value", "seq", "sysid"])

    t0, n, lost, last_seq = time.monotonic(), 0, 0, None
    print(f"listening on UDP {args.port} ...")
    while True:
        m = link.recv_match(blocking=True, timeout=5)
        if m is None:
            print("no packets for 5 s (link down?)"); continue
        hdr = m.get_header()
        if last_seq is not None:
            lost += (hdr.seq - last_seq - 1) % 256
        last_seq = hdr.seq
        kind = m.get_type()
        if kind == "NAMED_VALUE_FLOAT":
            n += 1
            if out:
                out.writerow([f"{time.monotonic()-t0:.3f}", m.time_boot_ms, m.value, hdr.seq, hdr.srcSystem])
            if n % 50 == 0:
                print(f"{n} samples, last={m.value:.2f}, lost so far={lost}")
        elif kind == "STATUSTEXT":
            print(f"ALERT from base station: {m.text}")


if __name__ == "__main__":
    main()
