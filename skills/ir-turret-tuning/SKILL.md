---
name: ir-turret-tuning
description: Diagnose and fix a CrunchLabs Hack Pack IR Turret that won't fire reliably - barrel stalls or spins slowly with darts loaded, only some darts fire, turret misses shots, remote feels laggy, or pitch won't reach full up/down. Drives the turret over USB serial to measure and tune it. Use when someone reports dart-firing, barrel-rotation, servo-stall or responsiveness problems with a Hack Pack / Mark Rober IR turret.
---

# IR Turret tuning

Diagnose and fix a CrunchLabs Hack Pack IR Turret by driving it over USB and measuring.

## Core principle

The firing path is **open-loop**. A continuous-rotation servo has no position feedback, so the code
runs it for a fixed time and hopes. **The board cannot sense the barrel.** Every rotation
measurement must come from the user's eyes. Never claim to have measured something the hardware
cannot report — in particular, a "timed fire" reports how long `delay()` ran, *not* how far
the barrel turned. Those numbers are identical whether the barrel spun or stalled.

## Step 1 — Establish the serial link

Find the port (CH340 chip, `1a86:7523`):

```bash
ls /dev/cu.usbserial-*        # macOS
ls /dev/ttyUSB*               # Linux
```

**Order matters.** Open the file descriptor *first*, then set `stty` — opening resets line settings,
so setting them first yields garbage at every baud rate:

```bash
exec 3<>/dev/cu.usbserial-XXXX
stty -f /dev/cu.usbserial-XXXX 115200 cs8 -cstopb -parenb raw -echo
( cat <&3 > out.txt & CP=$!; sleep 3; printf 's' >&3; sleep 1; kill $CP ); exec 3<&-
```

Opening the port **resets the board**. Expect the boot banner, and expect any live-tuned values to
revert to compiled defaults.

If the IDE reports "stuck compiling", check nothing is holding the port — the web IDE needs
exclusive WebSerial access. `lsof /dev/cu.usbserial-XXXX`.

## Step 2 — Upload the instrumented sketch

Use `IRTurret_FixedFiring.ino` from this repo. It adds single-character serial control
without touching the IR path. Keys: `s` status, `t` timed fire, `[`/`]` ±10 ms, `<`/`>` ±2 ms,
`k`/`K` ramp ±2, `z` reset magazine counter, `H`/`F` calibration spins, `T`/`B` pitch limits.

## Step 3 — Rule out power

Have the sketch print `millis()`. A brownout resets the chip, restarting uptime near zero.

Fire a loaded magazine and watch uptime. **Climbing monotonically = no brownout.** If it resets,
stop tuning — it's a power problem, and a 2 A+ supply is the fix.

Note the kit ships with a **USB battery pack**; USB *is* the normal power path. "I tried USB and it
was still slow" does **not** rule out power — a Nano's USB rail sits near 4.5 V behind a 500 mA
polyfuse, often weaker than batteries.

## Step 3.5 — Try the tape fix before tuning

CrunchLabs' troubleshooting guide recommends lining the back metal ring with one or two layers of
**frosted** Scotch tape. This cuts friction and spaces the magnets further apart, weakening the
holding force the roll servo must break away from at the start of every shot.

Suggest this **before** laddering timing values. Breakaway from a standstill is the hardest moment
in the firing cycle, and a barrel that sticks intermittently cannot be tuned reliably. If the tape
fixes the stall outright, no code change is needed.

## Step 4 — Separate rotation from release

Ask the user to mark the barrel with tape and mark the body.

- `H` spins one chamber's worth. Did it advance exactly one chamber?
- `F` spins a full turn's worth. Did the tape return to the mark?

**Barrel under-rotates** → timing problem, continue to Step 5.
**Barrel rotates correctly but darts don't launch** → peg or elastics. Stop tuning; it's mechanical.

## Step 5 — Ladder the base value

`rollStep` to 0. Six singles per magazine, ~3 s apart, counting hits. Try 200, 220, 240, 260.

Ask **which** shots missed:
- **Last shots** → cumulative under-rotation, raise the value
- **Scattered/middle** → past the tolerance edge, or you're in the noise
- **First shots** → base too low for a full magazine

## Step 6 — Add the ramp

The magazine lightens as it empties and the servo warms, so later shots usually need more time.
With the best base, try `rollStep` +6, +8, +10, +12.

Keep it gentle. Simulation across ramps −12…+12 shows a spread of ~0.1 darts, and the extremes are
consistently worst. Do not spend magazines arguing about ramp sign.

## Step 7 — Confirm, then bake in

**Run the winning setting twice.** Single magazines lie — the same setting produced 5/6 and then 4/6
in testing. Only after two consistent magazines, edit `rollPrecision` and `rollStep` in the sketch
and upload, so they survive a power cycle.

## Statistical discipline

This is the part that's easy to get wrong.

- One magazine is **n=1**. Differences of one dart are noise.
- Steps below ~10 ms are **below the noise floor** — 1 ms is ~0.24° against ~16° of per-shot variation.
- Do not tune to values you cannot reach with the available keys; say so rather than approximating silently.
- If several settings all land in the same band, say so plainly instead of ranking them.

## Other bugs worth checking

**Laggy remote.** The stock loop prints ~150 characters of IR diagnostics *before* acting, at 9600
baud — ~150 ms of blocking per press. Comment out `printIRResultShort` / `printIRSendUsage`.

**Pitch won't reach full travel.** The stock code discards a step that would exceed the limit
instead of clamping, so it stops up to `pitchMoveSpeed` degrees short. Use `min()`/`max()`.

**Nod shifts the aim.** `shakeHeadYes` moves `pitchServoVal` by 15° near a limit and never restores it.

**Stale commands.** IR keeps decoding during long blocking moves; flush after firing and gestures.

## Safety

The turret **fires darts**. Confirm the user's hands are clear and it's pointed somewhere safe
before sending any command that spins the barrel. Never fire on assumption — get an explicit go.
