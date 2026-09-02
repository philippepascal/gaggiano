# The screen's web page

Once the screen is on WiFi (Advanced, then WiFi on the panel), open
`http://gaggiano.local` on a phone or computer on the same network. No login.

| Path | What |
|---|---|
| `/` | status, three readings, a live chart of the last 150 s (same colors as the panel), download button |
| `/api/status` | JSON, one object: profile, notes, time, temp, pressure, valve, heater %, pump %, tempSet, pressSet, ctrlMode, link, the six button states, phase, timer, lastShot, heap |
| `/api/history` | JSON arrays of the last 300 samples (2 per second): temp, pressure, heater, pump, mode |
| `/logs.csv` | the SD card log as a download |

The page polls `/api/status` once a second and appends a sample every half second.

## Log file

`/gaggia/gaggia_logs.csv` on the SD card, started fresh at every boot. One row per
status line while the controller runs something (5 per second), one per second when
idle. Columns:

```
time,mode,temp,pressure,valve,heater,pump,tempSet,pressSet,pumpPct,linkOk,faults,counter
```

`time` is local ISO time once NTP has set the clock, `-` before that. `mode` is 0 off,
1 brew, 2 steam, 3 clean. `heater` and `pump` are percent. `counter` is the controller's
loop counter (10 ms per count); gaps mean lost status lines. Open it in a spreadsheet
and plot pressure against time for a shot.

Console: `SDLOG OFF` stops logging for the session.

## Where the page lives

`gaggiano-v2/web/index.html` is the source; `./gg build screen` embeds it into
`gaggiano-v2/web_page.h` (committed) with `tools/embed-web.py`.
