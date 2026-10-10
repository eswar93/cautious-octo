# telemetry-predictor

ESP32-U remote node -> (Wi-Fi, MAVLink/UDP) -> NUCLEO-F767ZI base station -> drop prediction + alerts.

```
 sensor --UART--> ESP32-U ==Wi-Fi/UDP/MAVLink==> router ==Ethernet==> Nucleo-F767ZI
                                                                       |  window -> features -> model
                  PC (Wireshark, capture.py, fake_remote.py) <---------+  STATUSTEXT alerts back
```

## What works right now (verified in the scaffold)
- `core/` C library + tests: `make -C core test` passes, and the C model output matches Python's.
- `ml/train.py`: trains a baseline on synthetic data (precision ~0.66, recall ~0.80 at threshold 0.7)
  and exports it to C.
That is on synthetic data only; the numbers say the pipeline works, not how it will do on your sensor.

## What is written but not yet compiled or run
ESP32 firmware, Nucleo glue, and PC tools could not be built in the environment this was generated in
(no ESP-IDF, no ARM toolchain, no pymavlink, no hardware). Expect small compile fixes on first build.

## Quick start (Windows 11)
1. `git init` this folder. Run `tools\fetch_mavlink.ps1`.
2. In WSL2: `pip install -r requirements.txt`, then `python tools/capture.py --csv data/t.csv` and, in a
   second terminal, `python tools/fake_remote.py` — you should see samples arriving.
3. Install ESP-IDF; `cd esp32_fw`, set Wi-Fi + destination IP via `idf.py menuconfig`, build, flash.
4. Follow `nucleo_fw/README.md` for CubeMX + the STM32 VS Code extension.
5. Work through `docs/ROADMAP.md`.

See `CLAUDE.md` for the context file Claude Code reads automatically.
