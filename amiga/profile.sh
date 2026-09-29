#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mode="${1:-probes}"
transition=0
case "$mode" in
  probes) flags=(PROBES=1); script=profile.gdb ;;
  fps) flags=(FPSCOUNT=1); script=profile_fps.gdb ;;
  trace)
    # Cycle-exact FS-UAE instruction traces of an FPS-only build, each
    # starting at a listed gameplay frame (one-based, as in FPS reports).
    trace_frames="${TRACE_FRAMES:-701}" trace_fields="${TRACE_FIELDS:-100}"
    [[ "$trace_frames" =~ ^[1-9][0-9]*(,[1-9][0-9]*)*$ && "$trace_fields" =~ ^[1-9][0-9]*$ ]] || {
      echo "TRACE_FRAMES must be a comma list of frames; TRACE_FIELDS a positive integer" >&2; exit 1; }
    (( trace_fields <= 100 )) || { echo "TRACE_FIELDS must not exceed 100" >&2; exit 1; }
    trace_count=$(tr ',' '\n' <<< "$trace_frames" | sort -un | wc -l | tr -d ' ')
    flags=(FPSCOUNT=1 TRACE_FRAMES="$trace_frames")
    trace_out="${TRACE_OUTPUT:-$PWD/../tmp/trace/run}"
    mkdir -p "$trace_out" .run/trace
    trace_out="$(cd "$trace_out" && pwd)"
    rm -f "$trace_out"/trace-*.bin
    sed -e "s|@COUNT@|$trace_count|" -e "s|@FIELDS@|$trace_fields|" -e "s|@OUTPUT@|$trace_out|" \
      profile_trace.gdb > .run/trace/profile_trace.gdb
    script=.run/trace/profile_trace.gdb
    ;;
  transition)
    transition=1
    flags=(PROBES=1); script=profile_transition.gdb
    export SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_FRAMES=0
    export PROFILE_CONTACT=0 PROFILE_LASER=0 PROFILE_NAVIGATE=0
    ;;
  *) echo "usage: bash amiga/profile.sh [probes|fps|trace|transition]" >&2; exit 1 ;;
esac
if [[ ${SKIP_LEVEL:-0} == 1 && $mode != trace ]]; then script=profile_clear.gdb; fi
# Always leave the playable binary behind, including after a timeout/failure.
restore() {
  make clean > .run/profile-restore.log 2>&1
  make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_LASER=0 PROFILE_CONTACT=0 PROFILE_FRAMES=0 PROFILE_TRANSITION=0 GENERATION_DETAIL=0 GENERATION_TIMING=0 PHASE_SAMPLING=0 TRACE_FRAMES= >> .run/profile-restore.log 2>&1
}
mkdir -p .run
if [[ ${PROFILE_CONTACT:-0} == 1 ]]; then
  env -u DOWNWELL_CONTACT_PROFILE_SEARCH make -C .. verify-contact-profile
fi
trap restore EXIT
make clean
make -j4 "${flags[@]}" PROFILE_AUTORUN=1 PROFILE_TRANSITION="$transition"
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
if [[ $mode == trace ]]; then
  ls "$trace_out"/trace-*.bin > /dev/null 2>&1 || { echo "FS-UAE wrote no trace" >&2; exit 1; }
  cp out/Downwell.elf "$trace_out/Downwell.elf"
  cp ".run/profile-${mode}.log" "$trace_out/capture.log"
  grep '^TRACE' "$trace_out/capture.log"
  echo "Traces: $trace_out (report: python3 tools/amiga-trace-report.py $trace_out)"
  exit 0
fi
sed -n '/PROFILE/,$p' ".run/profile-${mode}.log"
