# Bench checklist

Run items 1 and 2 after every flash. Run the whole list at each checkpoint of
`docs/REFACTOR-PLAN.md`. "Bench" means both boards on USB and wired to each other over
UART; nothing needs to be in the machine.

Console commands (`tools/serial-cmd.py controller <command>` or `... screen <command>`;
`tools/serial-cmd.py --list` shows ports). Controller: `VERSION`, `STATUS`, `LOG ON|OFF`,
`RX <protocol line>`, `HANG`, `DFU`. Screen: `VERSION`, `STATUS`, `LOG ON|OFF`, `SDLOG ON|OFF`.
With one data cable, put one board on USB at a time; either board powers the other
through the link connector.

| # | Step | Expect |
|---|---|---|
| 1 | `./gg detect` | both boards listed with a port |
| 2 | `VERSION` on the controller | the version string |
| 3 | `STATUS` on the controller | one line with mode, setpoints, readings, outputs, loop counter and max loop time (under 10 ms at rest) |
| 4 | Screen boots | main tab; temperature and pressure labels refresh about 5 times a second |
| 5 | Heat on from the screen | `STATUS` shows the temperature setpoint; boiler output above 0. Heat off: setpoint 0 |
| 6 | Brew on for 5 s | valve 1, pump output above 0, brew timer counting on screen. Brew off: valve 0, pump 0 |
| 7 | Steam on, then off; Clean on, then off | mode echoed in `STATUS` each time |
| 8 | Settings tab: change a value, Set, reboot the screen | value persists |
| 9 | Profile tab: duplicate, rename, select, delete | names correct after a screen reboot |
| 10 | Both boards running 10 minutes | screen prints `HEAP` lines every 10 s with a flat free value; controller loop counter still increasing; `STATUS` max loop time unchanged |
| 11 | Reboot the controller with NRST while the screen is heating | screen console shows `controller hello`, then `TUNE` and the heat `CMD` re-sent |
| 12 | Unplug the UART wire during a brew, wait 5 s, replug | controller console (`LOG ON`): `link timeout: mode off` within 3 s; on replug the next heartbeat restores brew within 1 s |
| 13 | Pull the pressure sensor's I2C wire (SDA or SCL) while heating | `STATUS`: `pressStale=1`, `pressFaults` counting, `i2cRecoveries` counting about every 2 s, `press` frozen at its last value, `linkOk=1`, loop counter still about 100 per second; screen keeps its buttons lit and shows no warning |
| 14 | Replug the I2C wire | within about 2 s `pressStale=0`, `press` live again, counters stop |
| 15 | Clean on with the I2C wire pulled | `STATUS`: mode 3, valve 1, `pumpOut=0.0` (the pump never runs on a stale pressure) |
| 16 | Unplug the UART wire while the screen heats | screen header shows `CONTROLLER SILENT` within 2 s; replug clears it |
| 17 | Power-cycle the machine 5 times (mains off, not just reset) and start a clean on each boot | pressure rises on every boot; `STATUS`: `zc=` climbing by about 100 per second whenever the mains is on |
| 18 | Steam with the wand closed, boiler at its setpoint | `STATUS`: `pumpOut=0.0` throughout (pressure above the max, no shot) |
| 19 | Open the wand while steaming at the setpoint | log: `pump` shows short bursts about `steam_gap_s` apart, none while `temp` is under `steam_min_temp`; `heater` 100 as soon as `temp` is 3 degrees under the setpoint; no water from the wand |

Record anything unexpected in the plan's notes log.
