# Roadmap

Status key: [x] scaffolded and tested here, [~] scaffolded, untested on hardware, [ ] to do

1. [x] Portable C++20 core (window, features, link monitor, predictor, debounce, pipeline) + host tests + Python parity
2. [x] Synthetic data + baseline logistic model exported to C
3. [~] ESP32 reads UART sensor, sends MAVLink/UDP — adapt `sensor_parse_line()` to your sensor
4. [~] PC tools: `fake_remote.py`, `capture.py`; inspect traffic in Wireshark
5. [~] Nucleo: CubeMX (ETH + lwIP + FreeRTOS) -> `udp_task` -> alerts as STATUSTEXT
6. [ ] Measure inference time (DWT->CYCCNT) and compare with prediction horizon
7. [ ] Log REAL data via capture.py, label real drop events, retrain (`ml/train.py` needs a CSV loader)
8. [ ] Lossy-link testing: `fake_remote.py --loss 0.1`, plus real RF distance/obstacles
9. [ ] BLE transport: ESP32 NimBLE GATT server + BLE-to-UART bridge into `app_on_bytes()`
10. [ ] Optional: small 1D CNN via ST Edge AI Core; compare against logistic baseline
11. [ ] Optional (separate): GStreamer pipeline on the PC if you add an ESP32-CAM

Suggested order: 4 (fake remote -> capture on PC) -> 5 -> 3 -> 6 -> 7 -> 8 -> 9.
Doing 4 first means every later piece has something known-good to talk to.
