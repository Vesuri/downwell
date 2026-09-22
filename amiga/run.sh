#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
. "${FSUAE_COMMON:-$HOME/.local/share/amiga/fsuae_common.sh}"

FSUAE="${FSUAE:-fs-uae}"
ROM="${1:-${KICKSTART:-$HOME/Documents/RetroPie/BIOS/kick31.rom}}"
EXE="${DOWNWELL_EXE:-out/Downwell.exe}"
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }
[ -f "$EXE" ] || { echo "not found: $EXE (build first)"; exit 1; }

RUN=.run
DH0="$RUN/dh0"
DH1="$RUN/dh1"
mkdir -p "$DH0/s" "$DH1" "$RUN/state"
printf 'cd dh1:\nDownwell\n' > "$DH0/s/startup-sequence"
cp -f "$EXE" "$DH1/Downwell"
echo "running $EXE"
rm -f "$RUN"/state/*.uss

# This fsemu-core FS-UAE takes a capture with HOST-KEY + S (hold F12,
# press S).  It reads the destination from the environment even though it
# also parses the command-line option.
SHOTS="${FSEMU_SCREENSHOTS_DIR:-$HOME/Pictures/Screenshots}"
mkdir -p "$SHOTS"
export FSEMU_SCREENSHOTS_DIR="$SHOTS"

fsuae_stop_previous
fsuae_track_self
exec "$FSUAE" \
  --amiga_model="${AMIGA_MODEL:-A500}" \
  --chip_memory="${CHIP_MEMORY:-512}" --fast_memory="${FAST_MEMORY:-8192}" \
  --kickstart_file="$ROM" \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=none --joystick_port_1=none \
  --automatic_input_grab=0 --fullscreen=0 \
  --window_width=760 --window_height=568 --ntsc_mode=0 \
  --state_dir="$RUN/state" \
  --screenshots_output_dir="$SHOTS"
