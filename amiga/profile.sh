#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mode="${1:-probes}"
case "$mode" in
  probes) flags=PROBES=1; script=profile.gdb ;;
  fps) flags=FPSCOUNT=1; script=profile_fps.gdb ;;
  *) echo "usage: bash amiga/profile.sh [probes|fps]" >&2; exit 1 ;;
esac
if [[ ${SKIP_LEVEL:-0} == 1 ]]; then script=profile_clear.gdb; fi
# Always leave the playable binary behind, including after a timeout/failure.
restore() {
  make clean > .run/profile-restore.log 2>&1
  make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_LASER=0 >> .run/profile-restore.log 2>&1
}
mkdir -p .run
trap restore EXIT
make clean
make -j4 "$flags" PROFILE_AUTORUN=1
printf 'Profile optimization: %s\n' "${AMIGA_OPTIMIZATION:--O3}"
printf 'Profile Laser Sight fixture: %s\n' "${PROFILE_LASER:-0}"
shasum -a 256 out/Downwell.exe
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-60}" \
GDB_SOURCE="$script" READY_MARKER=DOWNWELL_PROFILE_READY \
READY_DESCRIPTION="bounded scene profile" bash verify_startup.sh
cp .run/gdb-startup.log ".run/profile-${mode}.log"
sed -n '/PROFILE scene\|CLEAR PROFILE/,$p' ".run/profile-${mode}.log"
