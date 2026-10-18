#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "$FSUAE_COMMON"

ROM="${1:-$KICKSTART}"
EXE="${DOWNWELL_EXE:-out/Downwell.exe}"
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }
[ -f "$EXE" ] || { echo "not found: $EXE (build first)"; exit 1; }

RUN=.run
DH0="$RUN/dh0"
DH1="$RUN/dh1"

# The previous emulator still has these host directories mounted.  Stop it
# before replacing either the startup sequence or the executable, otherwise
# it can observe the new file while cp is still writing it (or leave the user
# looking at the old AmigaDOS prompt).
# DEBUG=1 also opens the gdb stub, on a port no other project holds.
fsuae_prepare

mkdir -p "$DH0/s" "$DH1" "$RUN/state"
printf 'Stack 8192\ncd dh1:\nDownwell\n' > "$DH0/s/startup-sequence"
cp -f "$EXE" "$DH1/Downwell"
cp -f "${EXE%.exe}.boot" "$DH1/Downwell.boot"
cp -f "${EXE%.exe}.front" "$DH1/Downwell.front"
cp -f "${EXE%.exe}.planes" "$DH1/Downwell.planes"
cp -f "${EXE%.exe}.game" "$DH1/Downwell.game"
cp -f "${EXE%.exe}.cavern" "$DH1/Downwell.cavern"
cp -f "${EXE%.exe}.cavernplanes" "$DH1/Downwell.cavernplanes"
for bank in results shop clear fields columns patterns exitmask selectionmask hud entrymask terrain wind encountermasks; do
  cp -f "${EXE%.exe}.$bank" "$DH1/Downwell.$bank"
done
echo "running $EXE"
rm -f "$RUN"/state/*.uss

# This fsemu-core FS-UAE takes a capture with HOST-KEY + S (hold F12,
# press S).  It reads the destination from the environment even though it
# also parses the command-line option.
SHOTS="${FSEMU_SCREENSHOTS_DIR:-$HOME/Pictures/Screenshots}"
mkdir -p "$SHOTS"
export FSEMU_SCREENSHOTS_DIR="$SHOTS"

# Model, memory, sound and window: fsuae_options in $FSUAE_COMMON.
KICKSTART="$ROM" AMIGA_MODEL="${AMIGA_MODEL:-A500}"
CHIP_KB="${CHIP_KB:-${CHIP_MEMORY:-512}}" FAST_KB="${FAST_KB:-${FAST_MEMORY:-8192}}"
fsuae_options
fsuae_exec \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=none --joystick_port_1=none \
  --window_width=760 --window_height=568 \
  --state_dir="$RUN/state" \
  --screenshots_output_dir="$SHOTS"
