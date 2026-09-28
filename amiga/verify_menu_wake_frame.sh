#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
case "${1:-wake}" in
    wake) capture_script=verify_menu_wake_frame.gdb; capture_marker=DOWNWELL_MENU_WAKE_FRAME_READY ;;
    all) capture_script=verify_garden_frames.gdb; capture_marker=DOWNWELL_GARDEN_FRAMES_READY ;;
    *) echo "Expected wake or all" >&2; exit 2 ;;
esac
mkdir -p .run
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-45}"
if ! [[ "$VERIFY_TIMEOUT_SECONDS" =~ ^[0-9]+$ ]] || (( VERIFY_TIMEOUT_SECONDS < 1 || VERIFY_TIMEOUT_SECONDS > 60 )); then
    echo "Garden verification requires a 1–60 second deadline" >&2
    exit 2
fi
restore() {
    local result=$?
    trap - EXIT
    if ! make clean > .run/menu-wake-restore.log 2>&1 ||
       ! make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 SCENE_VERIFY=0 >> .run/menu-wake-restore.log 2>&1; then
        echo "Normal build restoration failed; see .run/menu-wake-restore.log" >&2
        exit 20
    fi
    exit "$result"
}
trap restore EXIT
make clean
make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 SCENE_VERIFY=1
export VERIFY_TIMEOUT_SECONDS
GDB_SOURCE="$capture_script" READY_MARKER="$capture_marker" \
READY_DESCRIPTION="source-ordered hardware-scrolled garden" bash verify_startup.sh
