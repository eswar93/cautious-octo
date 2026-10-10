# Wire protocol (MAVLink 2, common dialect, UDP port 14550)

| Direction | Message | Purpose |
|---|---|---|
| remote -> base | `HEARTBEAT` @ 1 Hz, sysid 42 | link-loss detection (3 s timeout) |
| remote -> base | `NAMED_VALUE_FLOAT` name=`SENSOR`, `time_boot_ms` = sample time on the remote | the data stream |
| base -> remote/PC | `STATUSTEXT` (sysid 1) | alerts: `DROP PREDICTED p=..`, `drop risk cleared`, `LINK LOST`, `link restored` |

Rules
- Windows are built from `time_boot_ms` (sensor clock), never from packet arrival time.
- MAVLink `seq` gaps are counted as packet loss; the link monitor keeps loss separate from sensor behaviour.
- `time_boot_ms` is uint32 (wraps after ~49 days); `tw.c` uses unsigned subtraction so the wrap is safe (tested).
- Later: replace NAMED_VALUE_FLOAT with a small custom dialect if you add more channels.
