# CrunchLabs Hack Pack IR Turret Firing Fix and Tuning Kit

**Fixes the CrunchLabs / Mark Rober Hack Pack IR Turret when the barrel will not spin properly and darts do not fire.**

If your turret only fires 2 or 3 darts out of 6, if the barrel stalls or turns slowly when loaded,
or if it fires fine empty but bogs down with a full magazine, this repo has the fix.

Measured result on a turret that could only fire **2 of 5 darts**: **6 of 6, twice in a row.**

## Contents

* [Symptoms this fixes](#symptoms-this-fixes)
* [The root cause](#the-root-cause)
* [Try the tape fix first](#try-the-tape-fix-first)
* [Quick start](#quick-start)
* [Which version should you use](#which-version-should-you-use)
* [Every fix included](#every-fix-included)
* [Other things to check before you blame the code](#other-things-to-check-before-you-blame-the-code)
* [Tuning your turret](#tuning-your-turret)
* [Findings: why the turret would not fire](#findings-why-the-turret-would-not-fire)
* [The AI agent skill](#the-ai-agent-skill)
* [The tools folder](#the-tools-folder)
* [What is in here](#what-is-in-here)

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

> Your servo will differ. See [Tuning your turret](#tuning-your-turret) to find your own numbers.
> There is a repeatable procedure, and an AI agent skill that runs it for you.

## Try the tape fix first

This one is from CrunchLabs' own [turret troubleshooting guide](https://ide.crunchlabs.com/troubleshooting/turret),
and it is the fastest thing to try if your barrel will not spin.

**Line the back metal ring of the turret with one or two layers of Scotch tape.**
Use the **frosted** style rather than the clear style.

It works for two reasons at once:

1. The frosted surface **reduces friction** where the barrel rides on the ring.
2. The tape adds **space between the magnets**, which weakens their pull.

That second one matters more than it sounds. The magnets hold the barrel against the ring, and that
holding force is exactly what the roll servo has to overcome every time it starts from a standstill.
Weaken it slightly and the servo breaks away more easily, which is the single hardest moment in the
whole firing cycle.

Start with one layer. If the barrel still drags, try two. Too much tape and the barrel will sit
loose, so do not keep stacking it.

Do this before tuning any code. If it fixes the stall on its own, you may not need the timing
changes at all, and if you do still need them, you will be tuning against a barrel that moves
consistently instead of one that sometimes sticks.

## Quick start

1. Open the [CrunchLabs Hack Pack IDE](https://ide.crunchlabs.com/editor/ir-turret).
2. Paste in [`IRTurret_FixedFiring.ino`](IRTurret_FixedFiring.ino).
3. Upload. Load 6 darts. Fire.
4. Still missing darts? Follow [Tuning your turret](#tuning-your-turret). The sketch tunes live over USB, with no need to upload again.

## Which version should you use

Both sketches contain **exactly the same fixes** and the same tuned firing values, and both accept
live tuning over USB. The only difference is the passcode lock and how the remote buttons are mapped.

### IRTurret_FixedFiring.ino (root folder)

The one most people want. The turret is ready to use the moment it powers on.

| Button | Does |
|:--|:--|
| Arrows | Aim. Tap for fine aim, hold to travel faster. |
| OK | Fire one dart |
| `*` | Fire all six |
| `#` | Tell the turret you reloaded, so the firing ramp starts over |
| `1` / `2` | Nod yes / shake no |

### my-version/IRTurretMine.ino

Adds a **four digit passcode lock**. The turret boots **locked** and ignores every aiming and firing
button until the code is entered. Handy if you want it sitting on a shelf without anyone else
setting it off.

> **The passcode is CrunchLabs' design, not ours.** It comes from their official `passcode.ino`
> turret hack in [HackPackOfficial/HackPack-Code](https://github.com/HackPackOfficial/HackPack-Code),
> including the nod yes and shake no feedback. This version merges the firing and bug fixes from this
> repo into it. The nod and shake gestures themselves are in the stock sketch too, on buttons 1 and 2.

| Button | Does |
|:--|:--|
| `0` to `9` | Enter passcode digits while locked. Checks automatically on the fourth digit, with no enter key. |
| Arrows, OK | Aim and fire, **only once unlocked** |
| `#` | Fire all six, once unlocked |
| `*` | Lock it again |
| `1` / `2` | Nod yes / shake no, once unlocked |

A correct code makes the turret **nod yes**, a wrong one makes it **shake no**, and the buffer clears
either way so you can retry freely. Holding a number key is debounced so a single press cannot
register twice.

The default code is `2468`. Change it at the top of the sketch:

```cpp
#define PASSCODE_LENGTH 4          // must match the number of digits below
#define CORRECT_PASSCODE "2468"    // change this to your desired passcode
```

If you use a different number of digits, change `PASSCODE_LENGTH` to match or the check will never fire.

Two things to know. Because `*` becomes the lock button, **fire all six moves to `#`**, which is why
that version has no reload reset button. And this is a fun toy lock, not security. There is no
lockout on wrong guesses.

## Every fix included

| Fix | Problem it solves |
|:--|:--|
| `rollPrecision` 158 to 240 | Barrel under rotates, darts do not fire |
| Ramp per shot (`rollStep`) | Load changes as the magazine empties |
| Removed IR debug prints | About 150 ms of blocking serial output per button press, so the remote felt laggy |
| Pitch limits clamped | Turret stopped about 5 degrees short of full up and 7 degrees short of full down |
| Nod restores aim | CrunchLabs' `shakeHeadYes` shifted aim by 15 degrees near a limit and never restored it |
| IR buffer flush | Buttons pressed during a long move fired late |
| Repeat flag captured before `resume()` | Race that could double count or drop held presses |
| `yawPrecision` 150 to 70 | Left and right blocked for 150 ms per press |
| Live serial tuning | Change values over USB without uploading again |

## Other things to check before you blame the code

These cause the same symptoms.

* **Power.** The kit runs off a USB battery pack. Try a **2 A or better** supply. A Nano's USB path
  sits near 4.5 V and many banks cap at 1 A. In our case the board never browned out, since uptime
  never reset, but rule it out.
* **Elastics.** Over stretched, doubled, or hooked on the wrong nub raises the load a lot.
* **The release peg.** Bent inward, it fights every elastic.
* **Barrel drag.** Empty, it should spin freely by hand with the power off.

If you need **240 ms** to do what stock does in 158 ms, your roll servo is running at roughly
**two thirds of the speed the stock firmware assumes**. That is worth reporting to CrunchLabs support.
This repo makes the turret work, but it does not make a weak servo strong.

## Tuning your turret

Every servo is different. The numbers above worked on one turret. This is how to find yours.

### The two values

```cpp
int rollPrecision = 240;  // base run time per shot, ms
int rollStep      = 10;   // ms added per dart already fired
```

`fire()` runs the barrel for `rollPrecision + (rollStep * dartsFired)` ms. There is **no position
feedback**, so this is pure open loop timing. The number *is* the rotation.

### Tune it live over USB, with no uploading

Both sketches accept single character commands at **115200 baud**. Open the serial monitor
or any terminal and press keys:

| Key | Effect |
|:--|:--|
| `s` | Print status, all current values |
| `t` | Fire one dart, report the time used |
| `[` `]` | `rollPrecision` down 10 / up 10 |
| `<` `>` | `rollPrecision` down 2 / up 2 |
| `k` `K` | `rollStep` down 2 / up 2 |
| `z` | Reset the magazine counter after reloading |
| `H` | Spin one chamber's worth |
| `F` | Spin a full turn's worth |
| `T` `B` | Drive pitch to top / bottom and report the angle |
| `?` | Key list |

> Opening the serial port **resets the board**, so tuned values revert to the compiled defaults.
> Once you find numbers you like, edit them in the sketch and upload.

### Procedure

**1. Confirm it is a rotation problem.**
Load 6 darts, press `F` for a full turn's worth. Put tape on the barrel and a mark on the body first.
If the tape does not return to the mark, the barrel is under rotating, so keep going. If it lands
dead on but darts still do not fire, your problem is the **peg or the elastics**, not timing.

**2. Find the base.**
Set `rollStep` to 0 by pressing `k` until status shows 0. Load 6 darts, fire six singles with `t`,
count hits. Try 200, 220, 240, 260. **One magazine per setting.**

**3. Add the ramp.**
Take your best base and try `rollStep` of 6, 8, 10, 12. The magazine gets lighter as it empties,
so later shots usually need more time.

**4. Confirm.**
Run your best setting **twice**. Single magazines lie. We saw the same setting give 5 of 6 and then
4 of 6. Do not trust a result you have not reproduced.

**5. Bake it in.** Edit `rollPrecision` and `rollStep` in the sketch and upload.

### What good looks like

Six of six, twice running. If nothing gets you past about 5 of 6, you are likely at the mechanical
ceiling. See [Findings](#findings-why-the-turret-would-not-fire), and consider contacting CrunchLabs
about the roll servo.

### Do not bother with

* **Changing `rollMoveSpeed`.** It is already at maximum, `90 + 90 = 180`. There is no headroom.
* **Steps smaller than about 10 ms.** Per shot noise is far larger, so you are sampling randomness.
* **Ramp direction arguments.** We simulated it. Across ramps from minus 12 to plus 12 the spread is
  about 0.1 darts. Keep the ramp gentle and do not agonise over the sign.

## Findings: why the turret would not fire

A record of one debugging session, including the wrong turns.

### Starting symptom

Barrel spun freely empty. With 3 or fewer darts it fired. With 4 or more it stalled.
Stock sketch, `rollPrecision = 158`.

### Ruled out: brownout

The first theory was that a stalled servo was collapsing the 5 V rail and resetting the Nano.

**Test:** the sketch prints `millis()` on demand. A reset would restart uptime near zero.

**Result:** across every test, including a continuous 1488 ms loaded spin, **uptime never reset.**
Power delivery was never the failure. This eliminated a whole branch of investigation.

> Note: the AVR only resets *below* its brown out threshold. The rail could still sag enough to
> weaken the servo without rebooting the chip. This rules out the dramatic version, not all of it.

### Ruled out: rollMoveSpeed

```cpp
rollServo.write(rollStopSpeed + rollMoveSpeed);  // 90 + 90 = 180
```

180 is the end of the scale for a continuous rotation servo, and `write(180)` already emits the
widest pulse the Servo library sends, 2400 us. **There is no speed headroom.** CrunchLabs' own
comment says to keep it at 90 for maximum torque.

### The real mechanism: open loop cumulative drift

A continuous rotation servo has **no position feedback**. The code runs it for a fixed time and stops:

```cpp
rollServo.write(rollStopSpeed + rollMoveSpeed);
delay(rollPrecision);
rollServo.write(rollStopSpeed);
```

Time times speed equals rotation. If the servo is slow under load, the same time yields **less than
60 degrees**. And because nothing re references position, each shot's error **carries into the next**,
as a random walk.

Fitting a physical model to nine magazines of data:

```
per shot noise      sigma ~ 16 deg
peg tolerance       tol   ~ 40 deg
drift after 6 shots ~ 16 * sqrt(6) ~ 39 deg
```

**By the sixth shot, accumulated drift approaches the entire release window.** That is why misses
cluster at the end of a magazine, and why they move around when you change the timing.

### The Monte Carlo result, and where it was wrong

1,300 base and ramp combinations, plus a free form search over all six shot times independently:

```
BEST base=242 ramp=+0       expected 5.00/6
Best FLAT base=234          expected 4.99/6
Ramp advantage              +0.01 darts
UPPER BOUND (free form)     5.03/6
```

The model said **no schedule beats about 5 of 6**, and that ramp direction is worth about 0.01 darts.

**The model was wrong about the ceiling.** Real testing found `240 / +10` firing **6 of 6, twice**.
Most likely the fitted noise was too pessimistic, since two parameters railed against their search
bounds, a warning sign noted at the time. The simulation was useful for showing that ramp direction
barely matters and that steep ramps hurt, and wrong about the achievable maximum.
Trust the turret over the model.

### Measured data

| `rollPrecision` | `rollStep` | Shot times | Result |
|:--|:--|:--|:--|
| 158 (stock) | 0 | all 158 | 2 of 5 |
| 238 | 0 | all 238 | 5 of 6 |
| 248 | 0 | all 248 | 5 of 6, then 4 of 6 |
| 258 | 0 | all 258 | 5 of 6 |
| 264 | 0 | all 264 | 5 of 6 |
| 268 | 0 | all 268 | 3 of 6 |
| 236 | 2 | 236 to 246 | 4 of 6 |
| 240 | 8 | 240 to 280 | 5 of 6, 5 of 6 |
| **240** | **10** | **240 to 290** | **6 of 6, 6 of 6** |
| 240 | 12 | 240 to 300 | 4 of 6, 5 of 6 |

Note the spread *within* a single setting. 248 gave both 5 of 6 and 4 of 6. **Always confirm twice.**

### Other bugs found along the way

Unrelated to firing, but real.

**Pitch never reached its limits.** The step was rejected rather than clamped:

```cpp
if((pitchServoVal + pitchMoveSpeed) < pitchMax){ ... }   // discards the whole step
```

With `pitchMoveSpeed = 6` the turret stopped at 144 instead of 150. Fixed with `min()` and `max()`.

**A nod silently moved your aim.** `shakeHeadYes`, which is stock CrunchLabs code, shifts
`pitchServoVal` by 15 degrees to make room near a limit and never restores it. Verified on hardware: aim at 33 degrees, nod, still 33 after the fix.

**The remote was laggy.** Every button press printed about 150 characters of IR diagnostics at 9600
baud *before* acting. The transmit buffer fills and `Serial.print` blocks, so that is about **150 ms
of dead time per press**. Removing those prints was the single biggest responsiveness win.

**A race on the repeat flag.** `loop()` called `IrReceiver.resume()` and *then* read
`decodedIRData.flags`, which the receiver may already have overwritten.

### Verification

The logic was extracted and run against stubbed hardware on a desktop: **328 tests**, including an
exhaustive check of all 10,000 four digit passcodes and a 200,000 iteration randomized property test.
Mutation testing with 27 hand written mutants scores **85 percent**. The four survivors are two
equivalent mutants and two deliberately untested tuning constants.

Logic tests cannot catch timing, current draw or mechanics. Everything above was confirmed on the
real turret over USB.

## The AI agent skill

[`SKILL.md`](SKILL.md) is an [agent skill](https://docs.claude.com/en/docs/agents-and-tools/agent-skills)
that teaches an AI coding agent to debug your turret the same way this repo was produced. It drives
the turret over USB, rules out power, separates rotation problems from release problems, and ladders
the timing values to a confirmed setting.

**Install it for Claude Code:**

```bash
mkdir -p ~/.claude/skills/ir-turret-tuning
curl -o ~/.claude/skills/ir-turret-tuning/SKILL.md \
  https://raw.githubusercontent.com/elearningplugins/crunchlabs-ir-turret-fix/main/SKILL.md
```

Then just say what is wrong, for example *"my Hack Pack turret only fires 2 of 6 darts"*,
and the agent will pick the skill up. Other agents can use it too, since `SKILL.md` is plain
Markdown with YAML frontmatter that you can paste in as a system prompt.

**What it actually knows how to do:**

1. Open the serial link, including the file descriptor ordering that is needed on macOS.
   Setting the baud rate before opening the port gives garbage at every speed.
2. Rule out a brownout by watching `millis()` uptime for a reset during firing.
3. Suggest the frosted tape fix before spending any darts on tuning.
4. Separate under rotation from a peg or elastic problem using the calibration spins.
5. Ladder the base timing, and read *which* shots miss, since last shot misses and scattered
   misses mean different things.
6. Add the per shot ramp.
7. Require two consistent magazines before writing anything into the sketch.

It also carries the discipline that made the session work, and this is the part that matters most.
The board **cannot see the barrel**, so the user's eyes are the only sensor. A timed fire measures
how long `delay()` ran, **not** how far the barrel turned, and reads identically whether the barrel
spun or stalled. One magazine is a sample size of one. Steps below about 10 ms are beneath the noise
floor. And it must get an explicit go before firing anything, because the turret shoots darts.

## The tools folder

Optional. You do not need any of this to fix your turret. It is here so the numbers in this repo can
be checked and so you can repeat the work on your own turret.

| File | What it does | When you would use it |
|:--|:--|:--|
| `drive.sh` | Sends command keys to the turret over USB and prints everything it says back. Handles the port opening order and the boot delay for you. | Scripting a repeatable test instead of typing keys by hand. |
| `tests.cpp` | 328 logic tests: passcode state machine, pitch limit clamping, the firing ramp, buffer overflow safety, plus a 200,000 iteration randomized property test. | You changed the sketch and want to know you did not break anything. |
| `stubs.h`, `stubs.cpp` | Fake `Servo`, `IrReceiver` and `Serial`, plus a virtual clock, so the sketch compiles and runs on a normal computer with no Arduino attached. | Needed by `tests.cpp`. |
| `mutate.py` | Mutation testing. Makes 27 deliberate one line breaks in the sketch and checks the tests notice. Currently catches 85 percent. | Judging whether the tests are actually worth anything. |
| `model.py` | Fits a physical model of barrel rotation to real magazine results, estimating servo speed, load effects, peg tolerance and per shot noise. | Understanding why your turret misses, rather than guessing. |
| `optimize.py` | Monte Carlo search over base and ramp combinations using that fitted model. | Narrowing which settings are worth testing on real darts. |
| `bestramp.py` | Finds the best base for each ramp value at high precision. | Answering "does ramp direction even matter", which it turns out barely does. |

**Run the logic tests:**

```bash
cd tools
g++ -std=c++17 -I. tests.cpp stubs.cpp -o tests && ./tests
python3 mutate.py
```

`tests.cpp` includes a generated `sketch.cpp`, which is the `.ino` with its Arduino headers swapped
for the stubs. The comments at the top of `tests.cpp` show how to regenerate it.

**Drive the turret:**

```bash
./drive.sh /dev/cu.usbserial-1420 "s" "tttttt" "s"
```

That prints status, fires six single shots three seconds apart, then prints status again. Remember
that opening the port resets the board, so anything tuned live goes back to the compiled defaults.

A caveat on the simulation scripts: they were fitted to nine magazines from **one** turret, and the
model got the achievable ceiling wrong. See
[Findings](#findings-why-the-turret-would-not-fire). They are useful for ruling options out, not for
picking a final number. Only real darts do that.

## What is in here

| Path | What it is |
|:--|:--|
| [`IRTurret_FixedFiring.ino`](IRTurret_FixedFiring.ino) | **Start here.** All fixes, no passcode. Direct replacement for the stock sketch. |
| [`my-version/IRTurretMine.ino`](my-version/IRTurretMine.ino) | The author's build. Same fixes plus a passcode lock. |
| [`SKILL.md`](SKILL.md) | An **AI agent skill** that debugs your turret over USB. See [above](#the-ai-agent-skill). |
| [`tools/`](tools/) | Test harness, serial driver and simulation scripts. See [above](#the-tools-folder). |

## Keywords

CrunchLabs Hack Pack IR Turret fix, Mark Rober turret barrel will not spin, Hack Pack turret not firing,
IR turret only fires 2 darts, turret roll servo slow, rollPrecision tuning, barrel stalls with darts loaded,
Hack Pack IR turret troubleshooting, turret darts will not launch, IRTurret.ino fix, Arduino Nano turret servo stall

## License

MIT, see [LICENSE](LICENSE). Based on CrunchLabs' IRTurret control code
(2025 Crunchlabs LLC) and the IRremote library (2020 to 2022 Armin Joachimsmeyer).
Not affiliated with or endorsed by CrunchLabs.

**What is original here:** the firing timing work (`rollPrecision`, the per shot ramp and the
tuning method), the bug fixes listed above, the serial tuning interface, the test and simulation
tools, and the agent skill. **What is not:** the turret sketch itself, the passcode hack, the nod and
shake gestures, and the frosted tape tip, all of which are CrunchLabs'.
