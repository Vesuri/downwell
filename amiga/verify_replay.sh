#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"

FSUAE="${FSUAE:-fs-uae}"
GDB="${GDB:-m68k-amiga-elf-gdb}"
ROM="${KICKSTART:-$HOME/Documents/RetroPie/BIOS/kick31.rom}"
REFERENCE="${1:-../build/port-replay-reference}"
RUN=.run
DH0="$RUN/dh0"
DH1="$RUN/dh1"
GDBHOME="$RUN/gdbhome"
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-60}"
if ! [[ "$VERIFY_TIMEOUT_SECONDS" =~ ^[1-9][0-9]*$ ]]; then
  echo "VERIFY_TIMEOUT_SECONDS must be a positive integer"
  exit 1
fi

[ -x "$REFERENCE" ] || { echo "reference runner not found: $REFERENCE"; exit 1; }
[ -f out/Downwell.exe ] || { echo "verification build not found"; exit 1; }
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }

EXPECTED="$("$REFERENCE")"
mkdir -p "$DH0/s" "$DH1" "$RUN/state" "$GDBHOME"
printf 'Stack 8192\ncd dh1:\nDownwell\n' > "$DH0/s/startup-sequence"
cp -f out/Downwell.exe "$DH1/Downwell"
rm -f "$RUN"/state/*.uss "$RUN/gdb-out.log" "$RUN/fsuae-replay.log" \
  "$RUN/gdb-ready.marker"

fsuae_claim_port
cleanup() {
  if [ -n "${GDB_PID:-}" ]; then
    kill "$GDB_PID" 2>/dev/null || true
    wait "$GDB_PID" 2>/dev/null || true
  fi
  fsuae_stop
}
trap cleanup EXIT INT TERM
VERIFY_DEADLINE=$((SECONDS + VERIFY_TIMEOUT_SECONDS))

"$FSUAE" \
  --amiga_model=A500 --chip_memory=512 --fast_memory=8192 \
  --kickstart_file="$ROM" \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=none --joystick_port_1=none \
  --automatic_input_grab=0 --fullscreen=0 \
  --audio_driver=dummy --warp_mode=1 \
  --remote_debugger=20 --remote_debugger_port="$DEBUG_PORT" \
  --remote_debugger_trigger=Downwell --ntsc_mode=0 \
  --state_dir="$RUN/state" > "$RUN/fsuae-replay.log" 2>&1 &
FSUAE_PID=$!
fsuae_track "$FSUAE_PID"

for _ in $(seq 1 120); do
  if (( SECONDS >= VERIFY_DEADLINE )); then
    echo "verification exceeded ${VERIFY_TIMEOUT_SECONDS}s"
    exit 1
  fi
  kill -0 "$FSUAE_PID" 2>/dev/null || {
    echo "FS-UAE exited before opening its debugger stub"
    exit 1
  }
  if lsof -nP -iTCP:"$DEBUG_PORT" -sTCP:LISTEN >/dev/null 2>&1; then
    break
  fi
  sleep 0.25
done
lsof -nP -iTCP:"$DEBUG_PORT" -sTCP:LISTEN >/dev/null 2>&1 || {
  echo "FS-UAE debugger stub did not open"
  exit 1
}

env HOME="$GDBHOME" XDG_CACHE_HOME="$GDBHOME" \
  "$GDB" -q -batch \
  -ex "target remote 127.0.0.1:$DEBUG_PORT" \
  -x replay_verify.gdb out/Downwell.elf > "$RUN/gdb-out.log" 2>&1 &
GDB_PID=$!

while (( SECONDS < VERIFY_DEADLINE )); do
  if [ -f "$RUN/gdb-ready.marker" ]; then
    break
  fi
  if ! kill -0 "$GDB_PID" 2>/dev/null; then
    break
  fi
  sleep 0.1
done
if [ -f "$RUN/gdb-ready.marker" ]; then
  # The marker follows the stack guard and result print. GDB detach may
  # remain blocked in the stub; that is not a replay timeout. Stop the
  # debugger before the emulator and still validate the exact result below.
  kill "$GDB_PID" 2>/dev/null || true
  wait "$GDB_PID" 2>/dev/null || true
  GDB_PID=""
  fsuae_stop
elif kill -0 "$GDB_PID" 2>/dev/null; then
  kill "$GDB_PID" 2>/dev/null || true
  wait "$GDB_PID" 2>/dev/null || true
  echo "debugger did not complete replay within ${VERIFY_TIMEOUT_SECONDS}s"
  exit 1
else
  wait "$GDB_PID" 2>/dev/null || true
fi

ACTUAL="$(sed -n 's/^DOWNWELL_REPLAY_RESULT //p' "$RUN/gdb-out.log")"
if [ -z "$ACTUAL" ]; then
  echo "replay result missing from debugger output"
  tail -80 "$RUN/gdb-out.log"
  exit 1
fi
if [ "$ACTUAL" != "$EXPECTED" ]; then
  echo "cross-target replay mismatch"
  echo "host:  $EXPECTED"
  echo "Amiga: $ACTUAL"
  exit 1
fi

echo "cross-target replay passed: $ACTUAL"
