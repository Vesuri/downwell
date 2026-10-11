#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "$FSUAE_COMMON"

GDB="${GDB:-m68k-amiga-elf-gdb}"
ROM="$KICKSTART"
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

case "${REPLAY_FRAME_TRACE:-1}" in
  1) EXPECTED="$("$REFERENCE")" ;;
  0) EXPECTED="$("$REFERENCE" --no-frame-trace)" ;;
  *) echo "REPLAY_FRAME_TRACE must be 0 or 1"; exit 1 ;;
esac
mkdir -p "$DH0/s" "$DH1" "$RUN/state" "$GDBHOME"
printf 'Stack 8192\ncd dh1:\nDownwell\n' > "$DH0/s/startup-sequence"
cp -f out/Downwell.exe "$DH1/Downwell"
cp -f out/Downwell.boot "$DH1/Downwell.boot"
cp -f out/Downwell.front "$DH1/Downwell.front"
cp -f out/Downwell.planes "$DH1/Downwell.planes"
cp -f out/Downwell.game "$DH1/Downwell.game"
cp -f out/Downwell.cavern "$DH1/Downwell.cavern"
cp -f out/Downwell.cavernplanes "$DH1/Downwell.cavernplanes"
for bank in results shop clear fields columns patterns; do
  cp -f "out/Downwell.$bank" "$DH1/Downwell.$bank"
done
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

# Quiet debug run (no sound, window behind the others): fsuae_options in $FSUAE_COMMON.
KICKSTART="$ROM" AMIGA_MODEL=A500 CHIP_KB=512 FAST_KB=8192 WARP=1 DEBUG=1
fsuae_options
FSUAE_LOG="$RUN/fsuae-replay.log" fsuae_launch \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=none --joystick_port_1=none \
  --remote_debugger_trigger=Downwell \
  --state_dir="$RUN/state"

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
  -ex "set \$replay_frame_trace = ${REPLAY_FRAME_TRACE:-1}" \
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
  # Restoration rebuilds out/ immediately after this script exits. Keep the
  # exact failing image and symbols before that happens, not only its digest.
  FAILURE_DIR="$(mktemp -d "$RUN/replay-failure.XXXXXX")"
  cp out/Downwell.exe "$FAILURE_DIR/Downwell.exe"
  cp out/Downwell.elf "$FAILURE_DIR/Downwell.elf"
  cp "$RUN/gdb-out.log" "$FAILURE_DIR/gdb-out.log"
  printf '%s\n' "$EXPECTED" > "$FAILURE_DIR/expected.txt"
  printf '%s\n' "$ACTUAL" > "$FAILURE_DIR/actual.txt"
  shasum -a 256 "$FAILURE_DIR/Downwell.exe" "$FAILURE_DIR/Downwell.elf" > "$FAILURE_DIR/hashes.txt"
  echo "cross-target replay mismatch"
  echo "Failure artifacts retained in $FAILURE_DIR"
  echo "host:  $EXPECTED"
  echo "Amiga: $ACTUAL"
  exit 1
fi

echo "cross-target replay passed: $ACTUAL"
