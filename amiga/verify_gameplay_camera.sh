#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mkdir -p .run
restore() {
    make clean > .run/camera-restore.log 2>&1
    make -j4 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 CAMERA_VERIFY=0 SCENE_VERIFY=0 >> .run/camera-restore.log 2>&1
}
trap restore EXIT
make clean
make -j4 SKIP_TO_GAME=1 CAMERA_VERIFY=1
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-120}" \
GDB_SOURCE=verify_gameplay_camera.gdb READY_MARKER=DOWNWELL_GAMEPLAY_CAMERA_READY \
READY_DESCRIPTION="live horizontal camera transitions" bash verify_startup.sh
