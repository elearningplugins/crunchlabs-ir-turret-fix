# Tools

| File | What it does |
|---|---|
| `drive.sh` | Send command keys to the turret over USB and capture the replies. |
| `tests.cpp`, `stubs.h`, `stubs.cpp` | Logic test harness — runs the sketch against stubbed servos/IR on a desktop. |
| `mutate.py` | Mutation testing for the harness. |
| `model.py`, `optimize.py`, `bestramp.py` | Monte Carlo model of barrel rotation, fitted to real magazine data. |

## Run the tests

```bash
cd tools
# generate sketch.cpp from the .ino first - see comments in tests.cpp
g++ -std=c++17 -I. tests.cpp stubs.cpp -o tests && ./tests
python3 mutate.py
```

## Drive the turret

```bash
./drive.sh /dev/cu.usbserial-1420 "s" "tttttt" "s"
```

`drive.sh` opens the port (which **resets the board**), waits for boot, then sends each key group
with pauses. Values tuned live revert to compiled defaults on that reset.
