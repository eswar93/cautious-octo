# Project context for Claude Code

Learning project: ESP32-U "remote node" streams sensor telemetry (MAVLink 2 over UDP/Wi-Fi) to a
NUCLEO-F767ZI "base station" (Ethernet + lwIP) that predicts a significant drop in the sensor value
and sends alerts back as STATUSTEXT. Goals: learn WLAN/BLE, telemetry, MAVLink.

## Layout
- `core/`      portable header-only C++20 (no HAL, no heap, no exceptions/RTTI): `window`, `link_monitor`,
               `predictor`, `fixed_queue`, `pipeline` (packets/samples in -> alert events out). Host-tested.
- `esp32_fw/`  ESP-IDF project (Kconfig for Wi-Fi/IP/UART pins).
- `nucleo_fw/` CubeMX-generated project lives here; hand-written C++ only in `app/` (`BaseStation` = MAVLink adapter over `tp::Pipeline`; `udp_task.cpp`).
- `ml/`        synthetic data, features, training, export to `core/model_weights.h`.
- `tools/`     PC-side: fake_remote.py, capture.py, fetch_mavlink.*
- `proto/`     protocol spec (keep in sync with code).

## Rules
- Feature definitions exist in TWO places: `ml/features.py` and `core/window.hpp`. Change both, retrain
  (`python ml/train.py`), and run `make -C core test` — the golden-vector test must pass.
- `core/model_weights.hpp` and `core/test/golden.hpp` are GENERATED; never hand-edit.
- Embedded C++ rules: no exceptions, no RTTI, no heap/`new`, no `std::string`/`std::vector` in firmware code
  (host tests may use them), no function-local statics. Prefer `constexpr`, `std::array`, `std::span`,
  `std::optional`, `enum class`, `[[nodiscard]]`. See docs/CPP_NOTES.md.
- Use sensor timestamps (`time_boot_ms`), not arrival time, for windows.
- Never edit CubeMX-generated code outside USER CODE sections.
- Dev machine is Windows 11: embedded toolchain native, Python training in WSL2.

## Commands
- Core tests:  `make -C core test`   (needs g++ >= 10; use WSL2 or MSYS2 on Windows)
- Train:       `cd ml && python train.py`
- Fake remote: `python tools/fake_remote.py --host <ip> --loss 0.05`
- Capture:     `python tools/capture.py --csv data/run1.csv`
- ESP32:       `cd esp32_fw && idf.py set-target esp32 && idf.py menuconfig && idf.py build flash monitor`
