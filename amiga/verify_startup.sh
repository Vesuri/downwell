#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "$FSUAE_COMMON"

GDB="${GDB:-m68k-amiga-elf-gdb}"
ROM="$KICKSTART"
RUN=.run
DH0="$RUN/dh0"
DH1="$RUN/dh1"
GDBHOME="$RUN/gdbhome"
GDB_SOURCE="${GDB_SOURCE:-verify_startup.gdb}"
READY_MARKER="${READY_MARKER:-DOWNWELL_MENU_SPRITE_READY}"
READY_DESCRIPTION="${READY_DESCRIPTION:-splash and hardware-sprite menu loop}"
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-120}"
VERIFY_STACK_BYTES="${VERIFY_STACK_BYTES:-8192}"
VERIFY_CHIP_KB="${VERIFY_CHIP_KB:-512}"
case "$VERIFY_CHIP_KB" in
  512|1024) ;;
  *) echo "VERIFY_CHIP_KB must be 512 or 1024"; exit 1 ;;
esac
if ! [[ "$VERIFY_STACK_BYTES" =~ ^[1-9][0-9]*$ ]]; then
  echo "VERIFY_STACK_BYTES must be a positive integer" >&2
  exit 1
fi
if ! [[ "$VERIFY_TIMEOUT_SECONDS" =~ ^[1-9][0-9]*$ ]]; then
  echo "VERIFY_TIMEOUT_SECONDS must be a positive integer" >&2
  exit 1
fi

[ -f out/Downwell.exe ] || { echo "normal build not found"; exit 1; }
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }

# Claiming the debugger port also stops the prior project emulator.  Do that
# before touching its mounted host directories, for the same reason as the
# normal run script.
fsuae_claim_port

mkdir -p "$DH0/s" "$DH1" "$RUN/state" "$GDBHOME"
printf 'Stack %s\ncd dh1:\nDownwell\n' "$VERIFY_STACK_BYTES" > "$DH0/s/startup-sequence"
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
find "$RUN/state" -name '*.uss' -delete
rm -f "$RUN/gdb-startup.log" "$RUN/fsuae-startup.log" \
  "$RUN/gdb-ready.marker"

cleanup() {
  if [ -n "${GDB_PID:-}" ]; then
    kill "$GDB_PID" 2>/dev/null || true
    wait "$GDB_PID" 2>/dev/null || true
  fi
  fsuae_stop
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
VERIFY_DEADLINE=$((SECONDS + VERIFY_TIMEOUT_SECONDS))

# Quiet debug run (no sound, window behind the others): fsuae_options in $FSUAE_COMMON.
KICKSTART="$ROM" AMIGA_MODEL=A500 CHIP_KB="$VERIFY_CHIP_KB" FAST_KB=8192 WARP=1 DEBUG=1
echo "A500 verification: Chip=${CHIP_KB} KiB Fast=${FAST_KB} KiB"
fsuae_options
FSUAE_LOG="$RUN/fsuae-startup.log" fsuae_launch \
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

GDB_SCRIPT="$(fsuae_gdb_script "$GDB_SOURCE")"
env HOME="$GDBHOME" XDG_CACHE_HOME="$GDBHOME" \
  "$GDB" -q -batch -x "$GDB_SCRIPT" out/Downwell.elf \
  > "$RUN/gdb-startup.log" 2>&1 &
GDB_PID=$!

# FS-UAE's remote stub does not answer GDB's detach packet reliably. Let the
# breakpoint script publish its result first, then close the emulator side of
# the connection so GDB can terminate instead of leaving the verifier hung.
while (( SECONDS < VERIFY_DEADLINE )); do
  if [ -f "$RUN/gdb-ready.marker" ]; then
    fsuae_stop
    break
  fi
  if ! kill -0 "$GDB_PID" 2>/dev/null; then
    break
  fi
  sleep 0.1
done
if kill -0 "$GDB_PID" 2>/dev/null; then
  kill "$GDB_PID" 2>/dev/null || true
  wait "$GDB_PID" 2>/dev/null || true
  echo "debugger did not reach ${READY_DESCRIPTION} within ${VERIFY_TIMEOUT_SECONDS}s"
  tail -80 "$RUN/gdb-startup.log"
  exit 1
fi
wait "$GDB_PID" 2>/dev/null || true

grep -q "^${READY_MARKER}$" "$RUN/gdb-startup.log" || {
  echo "normal build did not reach ${READY_DESCRIPTION}"
  tail -80 "$RUN/gdb-startup.log"
  exit 1
}

echo "normal A500 ${READY_DESCRIPTION} passed"
