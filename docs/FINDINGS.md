# Findings: why the turret wouldn't fire

A record of one debugging session, including the wrong turns.

## Starting symptom

Barrel spun freely empty. With 3 or fewer darts it fired. With 4+ it stalled.
Stock sketch, `rollPrecision = 158`.

## Ruled out: brownout

The first theory was that a stalled servo was collapsing the 5 V rail and resetting the Nano.

**Test:** the sketch prints `millis()` on demand. A reset would restart uptime near zero.

**Result:** across every test — including a continuous 1488 ms loaded spin — **uptime never reset.**
Power delivery was never the failure. This eliminated a whole branch of investigation.

> Note: the AVR only resets *below* its brown-out threshold. The rail could still sag enough to
> weaken the servo without rebooting the chip. This rules out the dramatic version, not all of it.

## Ruled out: `rollMoveSpeed`

```cpp
rollServo.write(rollStopSpeed + rollMoveSpeed);  // 90 + 90 = 180
```

180 is the end of the scale for a continuous-rotation servo, and `write(180)` already emits the
widest pulse the Servo library sends (2400 µs). **There is no speed headroom.** CrunchLabs' own
comment says to keep it at 90 for maximum torque.

## The real mechanism: open-loop cumulative drift

A continuous-rotation servo has **no position feedback**. The code runs it for a fixed time and stops:

```cpp
rollServo.write(rollStopSpeed + rollMoveSpeed);
delay(rollPrecision);
rollServo.write(rollStopSpeed);
```

Time × speed = rotation. If the servo is slow under load, the same time yields **less than 60°**.
And because nothing re-references position, each shot's error **carries into the next** — a random walk.

Fitting a physical model to nine magazines of data:

```
per-shot noise      sigma ~ 16 deg
peg tolerance       tol   ~ 40 deg
drift after 6 shots ~ 16 * sqrt(6) ~ 39 deg
```

**By the sixth shot, accumulated drift approaches the entire release window.** That is why misses
cluster at the end of a magazine, and why they move around when you change the timing.

## The Monte Carlo result

1,300 base/ramp combinations, plus a free-form search over all six shot times independently:

```
BEST base=242 ramp=+0       expected 5.00/6
Best FLAT base=234          expected 4.99/6
Ramp advantage              +0.01 darts
UPPER BOUND (free-form)     5.03/6
```

The model said **no schedule beats ~5 of 6**, and that ramp direction is worth ~0.01 darts.

**The model was wrong about the ceiling.** Real testing found `240 / +10` firing **6 of 6, twice**.
Most likely the model's fitted noise was too pessimistic — two parameters railed against their
search bounds, a warning sign we noted at the time. Worth remembering: the simulation was useful
for showing that *ramp direction barely matters* and that *steep ramps hurt*, and wrong about the
achievable maximum. Trust the turret over the model.

## Measured data

| `rollPrecision` | `rollStep` | Shot times | Result |
|---|---|---|---|
| 158 (stock) | 0 | all 158 | 2/5 |
| 238 | 0 | all 238 | 5/6 |
| 248 | 0 | all 248 | 5/6, then 4/6 |
| 258 | 0 | all 258 | 5/6 |
| 264 | 0 | all 264 | 5/6 |
| 268 | 0 | all 268 | 3/6 |
| 236 | +2 | 236…246 | 4/6 |
| 240 | +8 | 240…280 | 5/6, 5/6 |
| **240** | **+10** | **240…290** | **6/6, 6/6** |
| 240 | +12 | 240…300 | 4/6, 5/6 |

Note the spread *within* a single setting (248 gave both 5/6 and 4/6). **Always confirm twice.**

## Bugs found along the way

Unrelated to firing, but real:

**Pitch never reached its limits.** The step was rejected rather than clamped:
```cpp
if((pitchServoVal + pitchMoveSpeed) < pitchMax){ ... }   // discards the whole step
```
With `pitchMoveSpeed = 6` the turret stopped at 144 instead of 150. Fixed with `min()`/`max()`.

**A nod silently moved your aim.** `shakeHeadYes` shifts `pitchServoVal` by 15° to make room near a
limit and never restores it. Verified on hardware: aim at 33°, nod, still 33° after the fix.

**The remote was laggy.** Every button press printed ~150 characters of IR diagnostics at 9600 baud
*before* acting — the transmit buffer fills and `Serial.print` blocks. That's **~150 ms of dead time
per press**. Removing those prints was the single biggest responsiveness win.

**A race on the repeat flag.** `loop()` called `IrReceiver.resume()` and *then* read
`decodedIRData.flags`, which the receiver may already have overwritten.

## Verification

The logic was extracted and run against stubbed hardware on a desktop: **328 tests**, including an
exhaustive check of all 10,000 four-digit passcodes and a 200,000-iteration randomized property test.
Mutation testing (27 hand-written mutants) scores **85%**; the four survivors are two equivalent
mutants and two deliberately-untested tuning constants.

Logic tests cannot catch timing, current draw or mechanics. Everything above was confirmed on the
real turret over USB.
