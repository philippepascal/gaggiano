# Screen / controller protocol, version 2

UART, 115200 8N1, 3.3 V. Controller USART2 (PA2 TX, PA3 RX); screen UART2 (GPIO17 TX,
GPIO18 RX). Text lines, one message per line, implemented once in
`libraries/GaggiaProtocol/` and used by both firmwares and by the host tests.

## Line format

```
$TYPE,field,field,...*HH\n
```

- `$` starts a message; anything before it on the line is ignored (boot noise).
- `TYPE` is one of the words below. Fields are separated by `,`.
- `*HH` is the checksum: XOR of every byte between `$` and `*` (both excluded), as two
  upper-case hex digits. A receiver drops a line whose checksum does not match.
- Numbers are decimal. Floats carry two decimals. Integers have no decimal point.
- A line is at most 120 bytes including `\n`. Longer lines are dropped whole.
- A receiver also drops a line whose type is unknown or whose field count is wrong.
  A dropped line never changes any state; it is only counted.

## Messages

| Type | Direction | Fields | When |
|---|---|---|---|
| `HELLO` | both | `version` (int), `firmware` (string, no commas, max 31 chars) | at boot. The controller also answers a `HELLO` with its own. |
| `STAT` | controller to screen | `mode` `temp` `pressure` `valve` `boilerOut` `pumpOut` `tempSet` `pressSet` `pumpPct` `linkOk` `faults` `counter` | every 200 ms |
| `CMD` | screen to controller | `mode` `tempSet` `pressSet` `pumpPct` | on change, and every 1000 ms as a heartbeat |
| `TUNE` | screen to controller | `bbRange` `pidCycle` `kp` `ki` `kd` `pumpStepUp` `pumpKp` `pumpKi` `pumpKd` | on change, and after every controller `HELLO` |

Field meanings:

- `mode`: 0 off (pump off, valve closed, boiler follows `tempSet`), 1 brew (pressure
  control to `pressSet`, valve open while `pressSet` > 0), 2 steam (pump duty capped at
  `pumpPct`, valve closed, `pressSet` is the ceiling), 3 clean (pump full on below
  `pressSet`, valve open).
- `temp` in degrees C, `pressure` in bar, `valve` 0/1, `boilerOut` 0..100 percent,
  `pumpOut` 0..127 (pulse-skip units), `tempSet` degrees C, `pressSet` bar, `pumpPct`
  percent.
- `linkOk`: 1 while the controller has received a valid `CMD` within the last 3 s.
- `faults`: count of rejected temperature readings since boot.
- `counter`: the controller's loop counter (10 ms per count); gaps mean lost lines.

## Rules

- Controller: if no valid `CMD` arrives for 3 s, it sets mode 0 (pump off, valve closed),
  keeps `tempSet` (the boiler stays hot), and reports `linkOk=0`. The next valid `CMD`
  restores normal operation.
- Screen: it sends the current `CMD` immediately when it changes and at least every
  1000 ms. If two consecutive `STAT` lines echo a mode or setpoint different from the
  last `CMD` sent, it re-sends the `CMD`.
- Screen: it sends `TUNE` when the values change and whenever it receives a `HELLO`
  from the controller (the controller rebooted and has default tuning).
- The screen's SD log stores the raw `STAT` lines.

## Example

```
$CMD,1,93.00,9.00,0.00*56
$STAT,1,92.85,8.97,1,34.0,88.40,93.00,9.00,0.00,1,0,123456*3A
```
