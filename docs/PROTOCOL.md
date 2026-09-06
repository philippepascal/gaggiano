# Screen / controller protocol, version 6

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
| `STAT` | controller to screen | `mode` `temp` `pressure` `valve` `boilerOut` `pumpOut` `tempSet` `pressSet` `pumpPct` `linkOk` `faults` `counter` `pressStale` `i2cRecoveries` `maxLoopMs` | every 200 ms |
| `CMD` | screen to controller | `mode` `tempSet` `pressSet` `pumpPct` | on change, and every 1000 ms as a heartbeat |
| `TUNE` | screen to controller | `bbRange` `pidCycle` `kp` `ki` `kd` `pumpStepUp` `pumpKp` `pumpKi` `pumpKd` `steamShotS` `steamGapS` `steamMinTemp` `pumpFlow` | on change, and after every controller `HELLO` |

Field meanings:

- `mode`: 0 off (pump off, valve closed, boiler follows `tempSet`), 1 brew (pressure
  control to `pressSet`, valve open while `pressSet` > 0), 2 steam (valve closed, steam
  assist: shots of pump at `pumpPct` for `steamShotS`, at least `steamGapS` apart, only
  while the pressure is below `pressSet`, which means the wand is open, and the boiler is
  at `steamMinTemp` or more; `pumpPct` 0 or `steamShotS` 0 disables. With the wand open
  the heater is also flat out below `tempSet` minus 3 degrees instead of the normal
  bang-bang band), 3 clean (pump full on below `pressSet`, valve open).
- `temp` in degrees C, `pressure` in bar, `valve` 0/1, `boilerOut` 0..100 percent,
  `pumpOut` 0..127 (pulse-skip units), `tempSet` degrees C, `pressSet` bar, `pumpPct`
  percent.
- `steamShotS`, `steamGapS` (v4), `steamMinTemp` (v5) in `TUNE`: steam assist timings in
  seconds and minimum boiler temperature in degrees C, from the profile's steam settings;
  the other `TUNE` fields come from the advanced settings.
- `pumpFlow` (v6, `TUNE`): the pump's flow at full range at brew pressure, ml/s, from the
  advanced settings. While brewing the controller adds a heater feed-forward for the
  incoming water (pump level times this flow times the rise from 20 C to `tempSet`,
  capped at 40 percent) on top of the PID; 0 disables it.
- `linkOk`: 1 while the controller has received a valid `CMD` within the last 3 s.
- `faults`: count of rejected sensor readings since boot (temperature readings out of
  range plus pressure reads that failed on I2C; the controller console `STATUS` splits
  them).
- `counter`: the controller's loop counter (10 ms per count); gaps mean lost lines. A
  counter that climbs slower than 100 per second means the loop is stalling.
- `pressStale` (v3): 1 while the controller has had no valid pressure reading for
  500 ms; the pump is held off in every mode until a reading comes back.
- `i2cRecoveries` (v3): how many times the controller has clocked the I2C bus free
  and re-initialised the pressure ADC since boot.
- `maxLoopMs` (v3): the longest loop pass since the previous `STAT`, in ms. Normally
  under 10.

## Rules

- Controller: if no valid `CMD` arrives for 3 s, it sets mode 0 (pump off, valve closed),
  keeps `tempSet` (the boiler stays hot), and reports `linkOk=0`. The next valid `CMD`
  restores normal operation.
- Screen: it sends the current `CMD` immediately when it changes and at least every
  1000 ms. If two consecutive `STAT` lines echo a mode or setpoint different from the
  last `CMD` sent, it re-sends the `CMD`.
- Screen: it sends `TUNE` when the values change and whenever it receives a `HELLO`
  from the controller (the controller rebooted and has default tuning).
- The screen's SD log stores the `STAT` fields and the screen's own events, see below.

## SD log

One CSV per screen boot under `/gaggia/logs`, downloadable from the web page. The
header row is:

```
time,mode,temp,pressure,valve,heater,pump,tempSet,pressSet,pumpPct,linkOk,faults,counter,pressStale,i2cRecoveries,maxLoopMs
```

- `STAT` rows: one per `STAT` while the controller runs something, one per second
  otherwise. `heater` is `boilerOut`; `pump` is `pumpOut` scaled to percent. `time` is
  `-` until the clock is set.
- Event rows: `<time>,#,<text>`. Written by the screen when a `CMD` is sent for a
  reason other than the heartbeat (`cmd change ...`, `cmd mismatch ...`, `cmd hello
  ...`), when `TUNE` is sent, when a controller `HELLO` arrives (a reboot), when
  lines are rejected (at most one row per second), when the controller goes silent or
  answers again, when it reports the link down while heartbeats are being sent, when
  the pressure reading goes stale or comes back, and at screen boot. The text has no
  commas; readers skip rows whose second field is `#`.

## Example

```
$CMD,1,93.00,9.00,0.00*56
$STAT,1,92.85,8.97,1,34.0,88.40,93.00,9.00,0.00,1,0,123456,0,0,6*..
```
