#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"

FSUAE="${FSUAE:-fs-uae}"
GDB="${GDB:-m68k-amiga-elf-gdb}"
ROM="${KICKSTART:-$HOME/Documents/RetroPie/BIOS/kick31.rom}"
RUN=.run
DH0="$RUN/dh0"
DH1="$RUN/dh1"
GDBHOME="$RUN/gdbhome"
GDB_SOURCE="${GDB_SOURCE:-verify_startup.gdb}"
READY_MARKER="${READY_MARKER:-DOWNWELL_MENU_SPRITE_READY}"
READY_DESCRIPTION="${READY_DESCRIPTION:-splash and hardware-sprite menu loop}"

[ -f out/Downwell.exe ] || { echo "normal build not found"; exit 1; }
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }

# Claiming the debugger port also stops the prior project emulator.  Do that
# before touching its mounted host directories, for the same reason as the
# normal run script.
fsuae_claim_port

mkdir -p "$DH0/s" "$DH1" "$RUN/state" "$GDBHOME"
printf 'cd dh1:\nDownwell\n' > "$DH0/s/startup-sequence"
cp -f out/Downwell.exe "$DH1/Downwell"
find "$RUN/state" -name '*.uss' -delete
rm -f "$RUN/gdb-startup.log" "$RUN/fsuae-startup.log"

cleanup() {
  fsuae_stop
}
trap cleanup EXIT INT TERM

"$FSUAE" \
  --amiga_model=A500 --chip_memory=512 --fast_memory=8192 \
  --kickstart_file="$ROM" \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=none --joystick_port_1=none \
  --automatic_input_grab=0 --fullscreen=0 \
  --audio_driver=dummy --warp_mode=1 \
  --remote_debugger=20 --remote_debugger_port="$DEBUG_PORT" \
  --remote_debugger_trigger=Downwell --ntsc_mode=0 \
  --state_dir="$RUN/state" > "$RUN/fsuae-startup.log" 2>&1 &
FSUAE_PID=$!
fsuae_track "$FSUAE_PID"

for _ in $(seq 1 120); do
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
  > "$RUN/gdb-startup.log" 2>&1

grep -q "^${READY_MARKER}$" "$RUN/gdb-startup.log" || {
  echo "normal build did not reach ${READY_DESCRIPTION}"
  tail -80 "$RUN/gdb-startup.log"
  exit 1
}

echo "normal A500 ${READY_DESCRIPTION} passed"
