# CrunchLabs Hack Pack IR Turret Firing Fix and Tuning Kit

**Fixes the CrunchLabs / Mark Rober Hack Pack IR Turret when the barrel will not spin properly and darts do not fire.**

If your turret only fires 2 or 3 darts out of 6, if the barrel stalls or turns slowly when loaded,
or if it fires fine empty but bogs down with a full magazine, this repo has the fix.

Measured result on a turret that could only fire **2 of 5 darts**: **6 of 6, twice in a row.**

## Symptoms this fixes

* Barrel **will not spin** or **stops spinning** when darts are loaded
* Turret fires with 1 to 3 darts loaded but **stalls with 4, 5 or 6**
* **Only some darts fire**, such as 2 of 6, 3 of 6, 4 of 6
* Barrel **turns slowly**, sounds strained, or **stops short** of the next chamber
* Darts **do not launch** even though the barrel moves
* `fireAll` works but **single shots miss**
* Remote feels **laggy** or unresponsive
* Turret **will not tilt all the way up** or all the way down

## The root cause

The stock sketch runs the roll servo for a fixed **158 ms** per shot and assumes that equals
1/6 of a rotation. It is **open loop**. A continuous rotation servo gives no position feedback,
so the code cannot tell how far the barrel actually turned.

If your servo is even slightly slow under load, 158 ms buys **less than 60 degrees**. The error
**accumulates** shot to shot, chambers drift out of line with the release peg, and darts stop firing.
The magazine also gets lighter as it empties, so a single fixed time cannot be right for all six shots.

**The fix** is a longer base time, plus a ramp that lengthens each shot as the magazine empties.

```cpp
int rollPrecision = 240;  // was 158, base run time per shot
int rollStep      = 10;   // ms added per dart already fired

int thisShot = rollPrecision + (rollStep * dartsFired);
```

Shot times become **240, 250, 260, 270, 280, 290 ms**.

> Your servo will differ. See [docs/TUNING.md](docs/TUNING.md) to find your own numbers. There is a
> repeatable procedure, and an AI agent skill that runs it for you.

## What is in here

| Path | What it is |
|:--|:--|
| [`IRTurret_FixedFiring.ino`](IRTurret_FixedFiring.ino) | **Start here.** All fixes, no passcode. Direct replacement for the stock sketch. |
| [`my-version/IRTurretMine.ino`](my-version/IRTurretMine.ino) | The author's build. Same fixes plus a four digit passcode lock. |
| [`docs/TUNING.md`](docs/TUNING.md) | How to find the right numbers for *your* servo. |
| [`docs/FINDINGS.md`](docs/FINDINGS.md) | The full investigation, data and dead ends. |
| [`skills/ir-turret-tuning/`](skills/ir-turret-tuning/) | An **AI agent skill** that reproduces the whole debugging session. |
| [`tools/`](tools/) | Serial driver script and the logic test harness. |

## Quick start

1. Open the [CrunchLabs Hack Pack IDE](https://ide.crunchlabs.com/editor/ir-turret).
2. Paste in [`IRTurret_FixedFiring.ino`](IRTurret_FixedFiring.ino).
3. Upload. Load 6 darts. Fire.
4. Still missing darts? Follow [docs/TUNING.md](docs/TUNING.md). The sketch tunes live over USB, with no need to upload again.

## Every fix included

| Fix | Problem it solves |
|:--|:--|
| `rollPrecision` 158 to 240 | Barrel under rotates, darts do not fire |
| Ramp per shot (`rollStep`) | Load changes as the magazine empties |
| Removed IR debug prints | About 150 ms of blocking serial output per button press, so the remote felt laggy |
| Pitch limits clamped | Turret stopped about 5 degrees short of full up and 7 degrees short of full down |
| Nod restores aim | A nod near a limit silently shifted aim by 15 degrees |
| IR buffer flush | Buttons pressed during a long move fired late |
| Repeat flag captured before `resume()` | Race that could double count or drop held presses |
| `yawPrecision` 150 to 70 | Left and right blocked for 150 ms per press |
| Live serial tuning | Change values over USB without uploading again |

## Before you blame the code

Check these first, because they cause the same symptoms.

* **Power.** The kit runs off a USB battery pack. Try a **2 A or better** supply. A Nano's USB path
  sits near 4.5 V and many banks cap at 1 A. In our case the board never browned out, since uptime
  never reset, but rule it out.
* **Elastics.** Over stretched, doubled, or hooked on the wrong nub raises the load a lot.
* **The release peg.** Bent inward, it fights every elastic.
* **Barrel drag.** Empty, it should spin freely by hand with the power off.

If you need **240 ms** to do what stock does in 158 ms, your roll servo is running at roughly
**two thirds of the speed the stock firmware assumes**. That is worth reporting to CrunchLabs support.
This repo makes the turret work, but it does not make a weak servo strong.

## Keywords

CrunchLabs Hack Pack IR Turret fix, Mark Rober turret barrel will not spin, Hack Pack turret not firing,
IR turret only fires 2 darts, turret roll servo slow, rollPrecision tuning, barrel stalls with darts loaded,
Hack Pack IR turret troubleshooting, turret darts will not launch, IRTurret.ino fix, Arduino Nano turret servo stall

## License

MIT, see [LICENSE](LICENSE). Based on CrunchLabs' IRTurret control code
(2025 Crunchlabs LLC) and the IRremote library (2020 to 2022 Armin Joachimsmeyer).
Not affiliated with or endorsed by CrunchLabs.
