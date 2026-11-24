#!/usr/bin/env bash
# WI.6: every start that refuses says why, from the Shell and from Workbench,
# and a small stack is not a refusal. Each case is one verify_startup.sh run
# of the normal build:
# - *-stack: started with a 4096-byte stack (Shell `Stack 4096`, an icon with
#   4096), the game runs on AmigaStart.c's own 8192 bytes, shows its menu and
#   quits on Escape with result 0 (tools/verify-workbench-start.py);
# - the others refuse (tools/verify-start-refusal.py, its docstring has the
#   checks): a missing data file, a short one, one with a wrong header, and
#   too little Chip memory (MACHINE=a500-512k-020), and a 68000
#   (MACHINE=a500).
# Run under the shared lock (make verify-amiga-refusals).
# VERIFY_REFUSAL_CASES picks cases by name.
set -euo pipefail
cd "$(dirname "$0")"

memory='Downwell needs 940 KB of Chip memory \([0-9]+ KB free\)\.'
cases=(
  # name                 environment
  "shell-stack           VERIFY_STACK_BYTES=4096"
  "shell-missing         VERIFY_SHELL_LOG=1 VERIFY_DATA_MISSING=shop"
  "shell-short           VERIFY_SHELL_LOG=1 VERIFY_DATA_SHORT=sndrun"
  "shell-header          VERIFY_SHELL_LOG=1 VERIFY_DATA_CORRUPT=tables"
  "shell-memory          VERIFY_SHELL_LOG=1 MACHINE=a500-512k-020"
  "shell-cpu             VERIFY_SHELL_LOG=1 MACHINE=a500"
  "workbench-stack       VERIFY_WORKBENCH=1 VERIFY_STACK_BYTES=4096"
  "workbench-missing     VERIFY_WORKBENCH=1 VERIFY_DATA_MISSING=tables"
  "workbench-short       VERIFY_WORKBENCH=1 VERIFY_DATA_SHORT=clear"
  "workbench-memory      VERIFY_WORKBENCH=1 MACHINE=a500-512k-020"
  "workbench-cpu         VERIFY_WORKBENCH=1 MACHINE=a500"
)
expected() {
  case "$1" in
    shell-missing) echo 'Cannot read data/Downwell\.shop: reinstall Downwell\.' ;;
    shell-short) echo 'Downwell\.sndrun is not the file Downwell needs: reinstall Downwell\.' ;;
    shell-header) echo 'Downwell\.tables is not the file Downwell needs: reinstall Downwell\.' ;;
    workbench-missing) echo 'Cannot read data/Downwell\.tables: reinstall Downwell\.' ;;
    workbench-short) echo 'data/Downwell\.clear is not the file Downwell needs: reinstall Downwell\.' ;;
    *-memory) echo "$memory" ;;
    *-cpu) echo 'Downwell needs a 68020 or later\.' ;;
  esac
}

passed=0
for entry in "${cases[@]}"; do
  read -r name settings <<< "$entry"
  if [ -n "${VERIFY_REFUSAL_CASES:-}" ] && [[ " $VERIFY_REFUSAL_CASES " != *" $name "* ]]; then continue; fi
  echo "== $name: $settings"
  case "$name" in
    *-stack)
      env $settings GDB=../tools/verify-workbench-start.py READY_MARKER=DOWNWELL_START_READY \
        READY_DESCRIPTION="start $name" bash verify_startup.sh
      grep -E '^(STACK|START) ' .run/gdb-startup.log ;;
    *)
      env $settings VERIFY_CASE="$name" VERIFY_REFUSAL="$(expected "$name")" \
        GDB=../tools/verify-start-refusal.py READY_MARKER=DOWNWELL_REFUSAL_READY \
        READY_DESCRIPTION="refusal $name" bash verify_startup.sh
      grep -E "^(REFUSAL|REQUESTER|ANSWER|START|AFTER|SHELL) " .run/gdb-startup.log ;;
  esac
  passed=$((passed + 1))
done
echo "start cases: $passed passed"
