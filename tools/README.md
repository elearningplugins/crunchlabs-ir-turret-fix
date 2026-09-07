# Tools

Optional. None of this is needed to fix a turret. See
[The tools folder](../README.md#the-tools-folder) in the main README for what each script does
and when you would reach for it.

Quick reference:

```bash
# logic tests, 328 checks against stubbed hardware
g++ -std=c++17 -I. tests.cpp stubs.cpp -o tests && ./tests

# mutation testing, 27 deliberate one line breaks
python3 mutate.py

# drive a real turret over USB
./drive.sh /dev/cu.usbserial-1420 "s" "tttttt" "s"
```

`tests.cpp` includes a generated `sketch.cpp`, which is the `.ino` with its Arduino headers replaced
by the stubs in this folder. Regenerate it whenever you change the sketch.
