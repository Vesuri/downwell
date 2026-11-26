#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
# INSTANCE=name (default: default) runs another emulator beside the first.
# Each instance has its own pidfile, disks, state and gdb port, so a run.sh
# stops only its own instance's previous emulator. It never touches the
# verification and profile scripts' emulators (they use .run itself), and
# never replaces the executable they have mounted.
INSTANCE="${INSTANCE:-default}"
case "$INSTANCE" in ""|*[!a-zA-Z0-9_-]*) echo "INSTANCE must be a plain name"; exit 1 ;; esac
RUN=".run/instances/$INSTANCE"
export FSUAE_RUN="$RUN"
# Default gdb port: per instance, and outside the range the verification
# scripts derive from this directory. An explicit $DEBUG_PORT still wins.
[ -n "${DEBUG_PORT:-}" ] || { export DEBUG_PORT=$(( 2745 + $(printf '%s' "$PWD/$INSTANCE" | cksum | cut -d' ' -f1) % 400 )); export FSUAE_PORT_PINNED=0; }
. "$FSUAE_COMMON"
. ./machine.sh

ROM="${1:-$KICKSTART}"
[ -f "$ROM" ] || { echo "Kickstart ROM not found: $ROM"; exit 1; }
PARTS="exe boot front planes game cavern cavernplanes results shop pause clear fields columns patterns exitmask selectionmask hud entrymask terrain encountermasks sndrun sndfront sndresults sndclear tables"

# The verification and profile scripts clean and rebuild out/ in their own
# configurations while they hold tmp/amiga.lock, and restore the playable
# build when they finish. Unless DOWNWELL_EXE names a build, the instance
# runs its own snapshot of out/, refreshed only while nobody holds the lock.
if [ -n "${DOWNWELL_EXE:-}" ]; then
  EXE="$DOWNWELL_EXE"
  [ -f "$EXE" ] || { echo "not found: $EXE"; exit 1; }
else
  EXE="$RUN/build/Downwell.exe"
  mkdir -p "$RUN/build" ../tmp
  exec 9>>../tmp/amiga.lock
  # Development-only defines (amiga-knobs.py's verify, profile, reference
  # and development sets): such a build needs more memory than the target.
  development='_VERIFY$|_AUDIT$|CENSUS|PROBE|FPSCOUNT|PROFILE_|PHASE_SAMPLING|GENERATION_|TRACE_FRAMES|FRAME_DUMP|SAMPLE_FRAME|REFERENCE_|COLLISION_OWNER|SCENE_COLLISION_BACKEND|FRAME_WORK_CANDIDATE|SKIP_'
  why=""
  if ! python3 ../tools/hold-lock.py 9 --try; then
    why="a check is rebuilding out/"
  elif [ ! -f out/Downwell.exe ] || [ ! -f out/Downwell.defines ]; then
    why="out/ has no recorded build"
  elif found=$(sed 's/=.*//' out/Downwell.defines | grep -E -m1 -- "$development"); then
    why="out/ holds a development build (${found#-D})"
  else
    for part in $PARTS; do cp -f "out/Downwell.$part" "$RUN/build/Downwell.$part.new"; done
    for part in $PARTS; do mv -f "$RUN/build/Downwell.$part.new" "$RUN/build/Downwell.$part"; done
  fi
  if [ -n "$why" ]; then
    if [ -f "$EXE" ]; then
      echo "$why; running this instance's snapshot from $(date -r "$EXE" '+%Y-%m-%d %H:%M')"
    else
      echo "$why and this instance has no playable snapshot yet: build one with 'make -C amiga' when no check is running"
      exit 1
    fi
  fi
  exec 9>&-
fi

DH0="$RUN/dh0"
DH1="$RUN/dh1"

# The previous emulator still has these host directories mounted.  Stop it
# before replacing either the startup sequence or the executable, otherwise
# it can observe the new file while cp is still writing it (or leave the user
# looking at the old AmigaDOS prompt).
# DEBUG=1 also opens the gdb stub, on a port no other project holds.
fsuae_prepare

mkdir -p "$DH0/s" "$DH1" "$RUN/state"
# The program reads PROGDIR:data/ before PROGDIR:; a stale data/ drawer would
# shadow the banks copied beside it.
if [ -d "$DH1/data" ]; then find "$DH1/data" -delete; fi
printf 'Stack 8192\ncd dh1:\nDownwell\n' > "$DH0/s/startup-sequence"
for part in $PARTS; do
  if [ "$part" = exe ]; then cp -f "$EXE" "$DH1/Downwell"; else cp -f "${EXE%.exe}.$part" "$DH1/Downwell.$part"; fi
done
# W1.10: save.ini persists in dh1 between runs; DOWNWELL_SAVE replaces it.
# The default instance carries the save over once from .run/dh1, which
# interactive runs used to share with the verification scripts.
if [ "$INSTANCE" = default ] && [ ! -f "$DH1/save.ini" ] && [ -f .run/dh1/save.ini ]; then
  cp -f .run/dh1/save.ini "$DH1/save.ini"
fi
if [ -n "${DOWNWELL_SAVE:-}" ]; then cp -f "$DOWNWELL_SAVE" "$DH1/save.ini"; fi
echo "running $EXE"
rm -f "$RUN"/state/*.uss

# This fsemu-core FS-UAE takes a capture with HOST-KEY + S (hold F12,
# press S).  It reads the destination from the environment even though it
# also parses the command-line option.
SHOTS="${FSEMU_SCREENSHOTS_DIR:-$HOME/Pictures/Screenshots}"
mkdir -p "$SHOTS"
export FSEMU_SCREENSHOTS_DIR="$SHOTS"

# Model and memory: MACHINE (machine.sh). Sound and window: fsuae_options.
downwell_machine || exit 1
echo "machine: $(downwell_machine_description)"
KICKSTART="$ROM"
fsuae_options
fsuae_exec \
  --uae_chipset="$MACHINE_CHIPSET" \
  --hard_drive_0="$DH0" --hard_drive_1="$DH1" \
  --joystick_port_0=none --joystick_port_1=none \
  --window_width=760 --window_height=568 \
  --state_dir="$RUN/state" \
  --screenshots_output_dir="$SHOTS"
