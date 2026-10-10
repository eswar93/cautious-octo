"""Pretend to be the ESP32 remote node: stream synthetic sensor data as MAVLink over UDP.

Lets you build and debug the Nucleo side (or capture.py / Wireshark) before the real radio
link works.  Example (target = your PC or the Nucleo's IP):

    python tools/fake_remote.py --host 192.168.1.50 --port 14550 --loss 0.05
"""
import argparse, pathlib, random, sys, time

from pymavlink import mavutil

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "ml"))
import synth  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=14550)
    ap.add_argument("--rate", type=float, default=10.0, help="sensor samples per second")
    ap.add_argument("--loss", type=float, default=0.0, help="random packet drop probability 0..1")
    ap.add_argument("--seed", type=int, default=7)
    args = ap.parse_args()

    link = mavutil.mavlink_connection(f"udpout:{args.host}:{args.port}",
                                      source_system=42, source_component=191)
    t, v, label = synth.generate(duration_s=600, rate_hz=args.rate, seed=args.seed)
    print(f"streaming to {args.host}:{args.port} @ {args.rate} Hz, loss={args.loss:.0%}  (Ctrl+C to stop)")

    t0, last_hb, i = time.monotonic(), 0.0, 0
    while True:
        now = time.monotonic() - t0
        if now - last_hb >= 1.0:
            link.mav.heartbeat_send(mavutil.mavlink.MAV_TYPE_GENERIC,
                                    mavutil.mavlink.MAV_AUTOPILOT_INVALID, 0, 0,
                                    mavutil.mavlink.MAV_STATE_ACTIVE)
            last_hb = now
        if i < len(v) and now >= t[i]:
            if random.random() >= args.loss:
                link.mav.named_value_float_send(int(t[i] * 1000), b"SENSOR", float(v[i]))
            else:
                link.mav.seq = (link.mav.seq + 1) % 256   # consume a seq number: receiver sees a gap
            if label[i] and (i == 0 or not label[i - 1]):
                print(f"[{now:7.1f}s] ground truth: drop expected within 2 s")
            i += 1
            if i >= len(v):
                i = 0; t0 = time.monotonic(); last_hb = 0.0
        time.sleep(0.002)


if __name__ == "__main__":
    main()
