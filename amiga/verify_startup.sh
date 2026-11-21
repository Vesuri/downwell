#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "$FSUAE_COMMON"
. ./machine.sh

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
VERIFY_PROGRAM="${VERIFY_PROGRAM:-out/Downwell}"
# VERIFY_CHIP_KB=512 is the older spelling of MACHINE=a500-512k.
case "${VERIFY_CHIP_KB:-}" in
  '') ;;
  512) MACHINE="${MACHINE:-a500-512k}" ;;
  1024) MACHINE="${MACHINE:-a500}" ;;
  *) echo "VERIFY_CHIP_KB must be 512 or 1024"; exit 1 ;;
esac
downwell_machine || exit 1
if ! [[ "$VERIFY_STACK_BYTES" =~ ^[1-9][0-9]*$ ]]; then
  echo "VERIFY_STACK_BYTES must be a positive integer" >&2
  exit 1
fi
if ! [[ "$VERIFY_TIMEOUT_SECONDS" =~ ^[1-9][0-9]*$ ]]; then
  echo "VERIFY_TIMEOUT_SECONDS must be a positive integer" >&2
  exit 1
fi

[ -f "$VERIFY_PROGRAM.exe" ] || { echo "verification executable not found: $VERIFY_PROGRAM.exe"; exit 1; }
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }

# Claiming the debugger port also stops the prior project emulator.  Do that
# before touching its mounted host directories, for the same reason as the
# normal run script.
fsuae_claim_port

mkdir -p "$DH0/s" "$DH1" "$RUN/state" "$GDBHOME"
printf 'Stack %s\ncd dh1:\nDownwell\n' "$VERIFY_STACK_BYTES" > "$DH0/s/startup-sequence"
cp -f "$VERIFY_PROGRAM.exe" "$DH1/Downwell"
cp -f out/Downwell.boot "$DH1/Downwell.boot"
cp -f out/Downwell.front "$DH1/Downwell.front"
cp -f out/Downwell.planes "$DH1/Downwell.planes"
cp -f out/Downwell.game "$DH1/Downwell.game"
cp -f out/Downwell.cavern "$DH1/Downwell.cavern"
cp -f out/Downwell.cavernplanes "$DH1/Downwell.cavernplanes"
for bank in results shop pause clear fields columns patterns exitmask selectionmask hud entrymask terrain encountermasks sndrun sndfront sndresults sndclear; do
  cp -f "out/Downwell.$bank" "$DH1/Downwell.$bank"
done
if [ "$VERIFY_PROGRAM" = out/WindAudioProbe ]; then
  cp -f out/Downwell.wind "$DH1/Downwell.wind"
fi
if [ "$VERIFY_PROGRAM" = out/PaulaMixerProbe ]; then
  cp -f out/Downwell.sfx "$DH1/Downwell.sfx"
fi
# W1.10: verification starts from a fresh save unless VERIFY_SAVE seeds one.
rm -f "$DH1/save.ini"
if [ -n "${VERIFY_SAVE:-}" ]; then cp -f "$VERIFY_SAVE" "$DH1/save.ini"; fi
find "$RUN/state" -name '*.uss' -delete
rm -f "$RUN/gdb-startup.log" "$RUN/fsuae-startup.log" \
  "$RUN/gdb-ready.marker" "$DH1/Downwell.reject-draw"

cleanup() {
  if [ -n "${GDB_PID:-}" ]; then
    kill "$GDB_PID" 2>/dev/null || true
    wait "$GDB_PID" 2>/dev/null || true
  fi
  fsuae_stop
  rm -f "$DH1/Downwell.reject-draw"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
VERIFY_DEADLINE=$((SECONDS + VERIFY_TIMEOUT_SECONDS))

# Quiet debug run (no sound, window behind the others): fsuae_options in $FSUAE_COMMON.
KICKSTART="$ROM" WARP=1 DEBUG=1
# VERIFY_AUDIO_FILE (an absolute path): record Paula's output instead, in
# real time, as raw 16-bit stereo through SDL's disk driver, with UAE's
# output filter and interpolation off and full stereo separation.
if [ -n "${VERIFY_AUDIO_FILE:-}" ]; then
  case "$VERIFY_AUDIO_FILE" in /*) ;; *) echo "VERIFY_AUDIO_FILE must be absolute"; exit 1 ;; esac
  rm -f "$VERIFY_AUDIO_FILE"
  WARP=0 SOUND=1
  export SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$VERIFY_AUDIO_FILE"
  EXTRA_ARGS="${EXTRA_ARGS:-} --audio_driver=sdl --uae_sound_filter=off --uae_sound_interpol=none --uae_sound_stereo_separation=10"
fi
echo "verification machine: $(downwell_machine_description)"
fsuae_options
FSUAE_LOG="$RUN/fsuae-startup.log" fsuae_launch \
  --uae_chipset="$MACHINE_CHIPSET" \
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
  "$GDB" -q -batch -x "$GDB_SCRIPT" "$VERIFY_PROGRAM.elf" \
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
  # The successful script writes its marker only after all target assertions.
  # Closing FS-UAE can race GDB's detach/exit. That is cleanup, not a timeout;
  # still require the exact success line below, as well as the file marker.
  if [ ! -f "$RUN/gdb-ready.marker" ]; then
    echo "debugger did not reach ${READY_DESCRIPTION} within ${VERIFY_TIMEOUT_SECONDS}s"
    tail -80 "$RUN/gdb-startup.log"
    exit 1
  fi
fi
wait "$GDB_PID" 2>/dev/null || true

grep -q "^${READY_MARKER}$" "$RUN/gdb-startup.log" || {
  echo "normal build did not reach ${READY_DESCRIPTION}"
  tail -80 "$RUN/gdb-startup.log"
  exit 1
}

echo "$MACHINE ${READY_DESCRIPTION} passed"
