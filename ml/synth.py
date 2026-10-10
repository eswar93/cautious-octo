"""Synthetic sensor with 'significant fall' events preceded by a detectable precursor.

Replace with real logged data once you have it (see tools/capture.py). The point of this
generator is to give you a working end-to-end pipeline before the hardware is connected.
"""
import numpy as np

BASELINE = 100.0
DROP_TO = 60.0


def generate(duration_s=1800, rate_hz=10, seed=0, horizon_s=2.0,
             event_every_s=(25, 60), precursor_s=3.0, noise=0.6):
    rng = np.random.default_rng(seed)
    n = int(duration_s * rate_hz)
    t = np.arange(n) / rate_hz
    v = BASELINE + 2.0 * np.sin(2 * np.pi * t / 40.0) + rng.normal(0, noise, n)
    label = np.zeros(n, dtype=np.int8)

    te = rng.uniform(*event_every_s)
    while te < duration_s - 12:
        i_e = int(te * rate_hz)
        i_p = int((te - precursor_s) * rate_hz)
        # precursor: accelerating decline of ~8 units over precursor_s
        ramp = np.linspace(0, 1, i_e - i_p) ** 1.5 * 8.0
        v[i_p:i_e] -= ramp
        # event: sharp fall, then exponential recovery over ~6 s
        k = np.arange(n - i_e) / rate_hz
        fall = (v[i_e - 1] - DROP_TO) * np.exp(-k / 3.0)
        fall[: int(0.3 * rate_hz)] = (v[i_e - 1] - DROP_TO)  # hold low briefly
        v[i_e:] -= fall[: n - i_e] * (k < 12.0)
        # label: windows ending within `horizon_s` BEFORE the event
        i_h = int((te - horizon_s) * rate_hz)
        label[i_h:i_e] = 1
        te += rng.uniform(*event_every_s)
    return t, v.astype(np.float32), label
