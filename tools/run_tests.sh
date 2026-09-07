#!/usr/bin/env bash
# Regenerate both sketches and run every test suite. Works from a fresh clone.
set -euo pipefail
cd "$(dirname "$0")"
echo "== generating =="
python3 generate.py
echo
echo "== passcode sketch (my-version/IRTurretMine.ino) =="
g++ -std=c++17 -I. tests.cpp stubs.cpp -o tests
./tests
echo
echo "== community sketch (IRTurret_FixedFiring.ino) =="
g++ -std=c++17 -I. community_tests.cpp stubs.cpp -o ctests
./ctests
echo
echo "== mutation testing (passcode sketch) =="
python3 mutate.py
