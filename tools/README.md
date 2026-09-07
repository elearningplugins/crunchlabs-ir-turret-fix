# Tools

Optional. None of this is needed to fix a turret. See
[The tools folder](../README.md#the-tools-folder) in the main README for what each script does.

## Run everything

```bash
./run_tests.sh
```

Regenerates both sketches, runs both logic suites, then mutation testing. Needs only `g++` and
`python3`, and works from a fresh clone. This is what CI runs.

## Pieces

* `generate.py` turns each `.ino` into a `.cpp` the harness can compile, swapping the Arduino
  headers for the stubs here. The generated files are not committed, so regenerate after editing a sketch.
* `tests.cpp` covers `my-version/IRTurretMine.ino`.
* `community_tests.cpp` covers `IRTurret_FixedFiring.ino`.
* `mutate.py` breaks the sketch 28 ways and checks the tests notice. A pattern that no longer matches
  the sketch fails the run rather than being skipped quietly.
* `drive.sh` talks to a real turret over USB.

```bash
./drive.sh /dev/cu.usbserial-1420 "s" "tttttt" "s"
```

Opening the port resets the board, so anything tuned live goes back to the compiled defaults.
