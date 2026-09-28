#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mkdir -p .run
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-45}"
if ! [[ "$VERIFY_TIMEOUT_SECONDS" =~ ^[0-9]+$ ]] || (( VERIFY_TIMEOUT_SECONDS < 1 || VERIFY_TIMEOUT_SECONDS > 60 )); then
    echo "Entry-wipe verification requires a 1–60 second deadline" >&2
    exit 2
fi
restore() {
    make clean > .run/entry-wipe-restore.log 2>&1
    make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 ENTRY_WIPE_VERIFY=0 >> .run/entry-wipe-restore.log 2>&1
}
trap restore EXIT
make clean
make -j4 SKIP_INTRO=0 SKIP_TO_GAME=1 SKIP_LEVEL=0 ENTRY_WIPE_VERIFY=1
export VERIFY_TIMEOUT_SECONDS
GDB_SOURCE=verify_entry_wipe.gdb READY_MARKER=DOWNWELL_ENTRY_WIPE_READY \
READY_DESCRIPTION="pointer-scrolled entry cover" bash verify_startup.sh
