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
# Always leave the playable binary behind, including after a timeout/failure.
restore() {
  make clean > .run/profile-restore.log 2>&1
  make -j4 >> .run/profile-restore.log 2>&1
}
mkdir -p .run
trap restore EXIT
make clean
make -j4 "$flags" PROFILE_AUTORUN=1
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-60}" \
GDB_SOURCE="$script" READY_MARKER=DOWNWELL_PROFILE_READY \
READY_DESCRIPTION="bounded 180-frame garden profile" bash verify_startup.sh
cp .run/gdb-startup.log ".run/profile-${mode}.log"
sed -n '/GARDEN/,$p' ".run/profile-${mode}.log"
