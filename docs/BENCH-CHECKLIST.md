# Bench checklist

Run items 1 and 2 after every flash. Run the whole list at each checkpoint of
`docs/REFACTOR-PLAN.md`. "Bench" means both boards on USB and wired to each other over
UART; nothing needs to be in the machine.

Console commands on the controller's USB port (`./gg monitor controller`, or
`tools/serial-cmd.py <port> <command>`): `VERSION`, `STATUS`, `LOG ON`, `LOG OFF`, `DFU`.

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
| 11 | Reboot the controller with NRST while the screen is heating | (from R2 on) screen re-sends settings and command without user action |
| 12 | Unplug the UART wire during a brew, wait 5 s, replug | (from R2 on) pump stops within 3 s; brew resumes only if still selected |

Record anything unexpected in the plan's notes log.
