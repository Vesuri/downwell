#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mode="${1:-probes}"
transition=0
case "$mode" in
  probes) flags=PROBES=1; script=profile.gdb ;;
  fps) flags=FPSCOUNT=1; script=profile_fps.gdb ;;
  transition)
    transition=1
    flags=PROBES=1; script=profile_transition.gdb
    export SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_FRAMES=0
    export PROFILE_CONTACT=0 PROFILE_LASER=0 PROFILE_NAVIGATE=0
    ;;
  *) echo "usage: bash amiga/profile.sh [probes|fps|transition]" >&2; exit 1 ;;
esac
if [[ ${SKIP_LEVEL:-0} == 1 ]]; then script=profile_clear.gdb; fi
# Always leave the playable binary behind, including after a timeout/failure.
restore() {
  make clean > .run/profile-restore.log 2>&1
  make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_LASER=0 PROFILE_CONTACT=0 PROFILE_FRAMES=0 PROFILE_TRANSITION=0 GENERATION_DETAIL=0 GENERATION_TIMING=0 PHASE_SAMPLING=0 >> .run/profile-restore.log 2>&1
}
mkdir -p .run
if [[ ${PROFILE_CONTACT:-0} == 1 ]]; then
  env -u DOWNWELL_CONTACT_PROFILE_SEARCH make -C .. verify-contact-profile
fi
trap restore EXIT
make clean
make -j4 "$flags" PROFILE_AUTORUN=1 PROFILE_TRANSITION="$transition"
printf 'Profile optimization: %s\n' "${AMIGA_OPTIMIZATION:--O3}"
printf 'Profile Laser Sight fixture: %s\n' "${PROFILE_LASER:-0}"
printf 'Profile natural contact tape: %s\n' "${PROFILE_CONTACT:-0}"
printf 'Profile inner generation probes: %s (only active in probes mode)\n' "${GENERATION_DETAIL:-0}"
printf 'Profile coarse generation timing: %s (FPS mode only)\n' "${GENERATION_TIMING:-0}"
printf 'Profile coarse timing stage: %s\n' "${GENERATION_TIMING_PHASE:-profileGeneration}"
shasum -a 256 out/Downwell.exe
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-60}" \
GDB_SOURCE="$script" READY_MARKER=DOWNWELL_PROFILE_READY \
READY_DESCRIPTION="bounded scene profile" bash verify_startup.sh
cp .run/gdb-startup.log ".run/profile-${mode}.log"
if [[ ${GENERATION_TIMING:-0} == 1 ]] && ! grep -q '^PHASE coarse:' ".run/profile-${mode}.log"; then
  echo "Requested coarse generation measurement was not produced" >&2
  exit 1
fi
if [[ ${PHASE_SAMPLING:-0} == 1 ]] && ! grep -q '^SAMPLE total=' ".run/profile-${mode}.log"; then
  echo "Requested phase samples were not produced" >&2
  exit 1
fi
sed -n '/PROFILE/,$p' ".run/profile-${mode}.log"
