"""Window features. MUST match core/window.hpp exactly (same order, same definitions)."""
import numpy as np

NFEAT = 5
FEATURE_NAMES = ["mean", "std", "slope", "last_minus_first", "last"]


def window_features(t_s: np.ndarray, v: np.ndarray) -> np.ndarray:
    """t_s: sensor timestamps in seconds (monotonic), v: values. Returns shape (NFEAT,)."""
    t = t_s - t_s[0]
    mean_v, mean_t = v.mean(), t.mean()
    dv, dt = v - mean_v, t - mean_t
    sxx = float((dt * dt).sum())
    slope = float((dt * dv).sum() / sxx) if sxx > 0 else 0.0
    return np.array([mean_v, v.std(), slope, v[-1] - v[0], v[-1]], dtype=np.float64)


def make_windows(t_s, v, label, n):
    """Slide a window of n samples; label of a window = label at its last sample."""
    X, y = [], []
    for end in range(n, len(v) + 1):
        sl = slice(end - n, end)
        X.append(window_features(t_s[sl], v[sl]))
        y.append(label[end - 1])
    return np.array(X), np.array(y)
