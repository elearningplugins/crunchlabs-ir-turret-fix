# Tuning your turret

Every servo is different. These are the numbers that worked on one turret; yours may differ.
This is the procedure to find yours.

## The two values

```cpp
int rollPrecision = 240;  // base run time per shot, ms
int rollStep      = 10;   // ms added per dart already fired
```

`fire()` runs the barrel for `rollPrecision + (rollStep * dartsFired)` ms. There is **no position
feedback**, so this is pure open-loop timing — the number *is* the rotation.

## Tune it live over USB (no re-uploading)

Both sketches accept single-character commands at **115200 baud**. Open the serial monitor
(or any terminal) and press keys:

| Key | Effect |
|---|---|
| `s` | Print status — all current values |
| `t` | Fire one dart, report the time used |
| `[` `]` | `rollPrecision` −10 / +10 |
| `<` `>` | `rollPrecision` −2 / +2 |
| `k` `K` | `rollStep` −2 / +2 |
| `z` | Reset the magazine counter (after reloading) |
| `H` | Spin one chamber's worth |
| `F` | Spin a full turn's worth |
| `T` `B` | Drive pitch to top / bottom and report the angle |
| `?` | Key list |

> Opening the serial port **resets the board**, so tuned values revert to the compiled defaults.
> Once you find numbers you like, edit them in the sketch and upload.

## Procedure

**1. Confirm it's a rotation problem.**
Load 6 darts, press `F` (a full turn's worth). Put a piece of tape on the barrel and a mark on the
body first. If the tape doesn't return to the mark, the barrel is under-rotating — keep going.
If it lands dead-on but darts still don't fire, your problem is the **peg or the elastics**, not timing.

**2. Find the base.**
Set `rollStep` to 0 (`k` until status shows 0). Load 6 darts, fire six singles with `t`, count hits.
Try 200, 220, 240, 260. **One magazine per setting.**

**3. Add the ramp.**
Take your best base and try `rollStep` of +6, +8, +10, +12. The magazine gets lighter as it empties,
so later shots usually need more time.

**4. Confirm.**
Run your best setting **twice**. Single magazines lie — we saw the same setting give 5/6 and then 4/6.
Don't trust a result you haven't reproduced.

**5. Bake it in.** Edit `rollPrecision` and `rollStep` in the sketch and upload.

## What "good" looks like

Six of six, twice running. If nothing gets you past ~5 of 6, you're likely at the
mechanical ceiling — see [FINDINGS.md](FINDINGS.md) for why, and consider contacting CrunchLabs
about the roll servo.

## Don't bother with

- **Changing `rollMoveSpeed`.** It's already at maximum (`90 + 90 = 180`). There is no headroom.
- **Steps smaller than ~10 ms.** Per-shot noise is far larger; you're sampling randomness.
- **Ramp direction arguments.** We simulated it: across ramps from −12 to +12, the spread is
  about 0.1 darts. Keep the ramp gentle and don't agonise over the sign.
