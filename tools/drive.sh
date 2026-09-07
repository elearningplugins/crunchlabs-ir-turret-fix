#!/usr/bin/env bash
# Drive the IR turret over USB serial and capture its replies.
#   ./drive.sh /dev/cu.usbserial-1420 "2468z" "tttttt"
# Each argument after the port is a string of command keys; 3s pause between groups.
set -euo pipefail
PORT="${1:?usage: drive.sh <port> [keygroup...]}"; shift
OUT="$(mktemp)"
exec 3<>"$PORT"
stty -f "$PORT" 115200 cs8 -cstopb -parenb raw -echo
cat <&3 > "$OUT" & CP=$!
sleep 3                                    # board resets on port open; wait for boot
for group in "$@"; do
  for (( i=0; i<${#group}; i++ )); do
    printf '%s' "${group:$i:1}" >&3
    sleep 0.4
  done
  sleep 3
done
sleep 1; kill $CP 2>/dev/null || true
exec 3<&-
LC_ALL=C tr -d '\r' < "$OUT"
