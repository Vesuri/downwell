#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mode="${1:-probes}"
transition=0
case "$mode" in
  probes) flags=(PROBES=1); script=profile.gdb ;;
  fps) flags=(FPSCOUNT=1); script=profile_fps.gdb ;;
  overflow-state)
    flags=(FPSCOUNT=1 OVERFLOW_VERIFY=1); script=verify_overflow.gdb
    export PROFILE_OVERFLOW=1 SKIP_TO_GAME=1 SKIP_INTRO=0 SKIP_LEVEL=0
    export PROFILE_CONTACT=0 PROFILE_ENTRY=0 PROFILE_TRANSITION=0 PROFILE_LASER=0 PROFILE_FRAMES=0 PROFILE_SHOP=0
    export VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-180}"
    ;;
  shop|terrain-shop)
    flags=(FPSCOUNT=1); script=profile_shop.gdb
    if [[ ${PROBES:-0} == 1 ]]; then flags=(PROBES=1 FPSCOUNT=0); fi
    if [[ $mode == terrain-shop ]]; then
      flags=(FPSCOUNT=1 TERRAIN_BATCH=1 TERRAIN_BATCH_VERIFY=1)
      script=verify_terrain_shop.gdb
    fi
    export PROFILE_SHOP=1 SKIP_INTRO=0 SKIP_TO_GAME=1 SKIP_LEVEL=0 PROFILE_FRAMES=0
    export PROFILE_CONTACT=0 PROFILE_ENTRY=0 PROFILE_TRANSITION=0 PROFILE_LASER=0
    export VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-600}"
    ;;
  draw-cache) flags=(FPSCOUNT=1 DRAW_CACHE_AUDIT=1); script=profile_draw_cache.gdb ;;
  draw-order) flags=(FPSCOUNT=1 DRAW_ORDER_AUDIT=1); script=profile_draw_order.gdb ;;
  ammo) flags=(FPSCOUNT=1 AMMO_FACTORED=2 AMMO_FACTORED_VERIFY=1); script=verify_ammo.gdb ;;
  terrain) flags=(FPSCOUNT=1 TERRAIN_BATCH=1 TERRAIN_BATCH_VERIFY=1); script=verify_terrain_batch.gdb ;;
  fields) flags=(FPSCOUNT=1 PREPARED_FIELDS=1 PREPARED_FIELDS_VERIFY=1); script=verify_prepared_fields.gdb ;;
  fields-clear)
    flags=(FPSCOUNT=1 PREPARED_FIELDS=1 PREPARED_FIELDS_VERIFY=1)
    script=verify_prepared_fields_clear.gdb
    export SKIP_TO_GAME=1 SKIP_LEVEL=1 PROFILE_CONTACT=0
    # Exhaustive scalar pixel oracles run at both startup and reentry.
    # This is a correctness run, not an uninstrumented timing sample.
    export VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-600}"
    ;;
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
  frames)
    # Save the published gameplay buffer at each listed tick (decode with
    # tools/decode-amiga-gameplay-frame.py); for presentation regressions.
    dump_ticks="${DUMP_TICKS:-640}"
    [[ "$dump_ticks" =~ ^[0-9]+(,[0-9]+)*$ ]] || { echo "DUMP_TICKS must be a comma list of ticks" >&2; exit 1; }
    flags=(FPSCOUNT=1 FRAME_DUMP=1)
    dump_out="${DUMP_OUTPUT:-$PWD/../tmp/frames}"
    mkdir -p "$dump_out" .run/frames
    dump_out="$(cd "$dump_out" && pwd)"
    {
      printf 'set pagination off\nset confirm off\ntarget remote 127.0.0.1:2345\n'
      for tick in $(tr ',' '\n' <<< "$dump_ticks" | sort -un); do
        printf 'tbreak downwellFrameDump if gDownwellDumpTick == %s\ncontinue\n' "$tick"
        printf 'dump binary memory %s/frame-%s.bin &gDownwellDumpRingOrigin &gDownwellDumpRingOrigin+1\n' "$dump_out" "$tick"
        printf 'append binary memory %s/frame-%s.bin gDownwellDumpGame gDownwellDumpGame+30720\n' "$dump_out" "$tick"
        printf 'append binary memory %s/frame-%s.bin gDownwellDumpHud gDownwellDumpHud+27264\n' "$dump_out" "$tick"
        printf 'append binary memory %s/frame-%s.bin gDownwellDumpPlayerSprite gDownwellDumpPlayerSprite+68\n' "$dump_out" "$tick"
        printf 'append binary memory %s/frame-%s.bin gDownwellDumpActorSprites gDownwellDumpActorSprites+408\n' "$dump_out" "$tick"
        printf 'append binary memory %s/frame-%s.bin gDownwellDumpOverflowSprite gDownwellDumpOverflowSprite+68\n' "$dump_out" "$tick"
      done
      printf 'printf "HUD_WORD_FALLBACKS=%%lu\\n", gDownwellHudWordFallbacks\nif gDownwellHudWordFallbacks != 0\nquit 1\nend\n'
      printf 'printf "DOWNWELL_PROFILE_READY\\n"\nshell touch .run/gdb-ready.marker\ndetach\nquit\n'
    } > .run/frames/dump.gdb
    script=.run/frames/dump.gdb
    ;;
  transition)
    transition=1
    flags=(PROBES=1); script=profile_transition.gdb
    export SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_FRAMES=0
    export PROFILE_CONTACT=0 PROFILE_LASER=0 PROFILE_NAVIGATE=0
    ;;
  entry)
    flags=(PROBES=1); script=profile_entry.gdb
    entry_ammo_verify=0
    if [[ ${AMMO_FACTORED_VERIFY:-0} == 1 ]]; then entry_ammo_verify=1; fi
    mkdir -p .run/entry
    sed "s/@VERIFY_AMMO@/$entry_ammo_verify/" profile_entry.gdb > .run/entry/profile_entry.gdb
    script=.run/entry/profile_entry.gdb
    export PROFILE_ENTRY=1 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_FRAMES=0
    export PROFILE_CONTACT=0 PROFILE_LASER=0 PROFILE_NAVIGATE=0
    ;;
  *) echo "usage: bash amiga/profile.sh [probes|fps|overflow-state|shop|terrain-shop|trace|frames|draw-cache|draw-order|ammo|transition|entry|terrain|fields|fields-clear]" >&2; exit 1 ;;
esac
if [[ $mode == draw-cache || $mode == draw-order || $mode == ammo ]]; then
  export SKIP_TO_GAME=1 SKIP_LEVEL=0 PROFILE_CONTACT="${PROFILE_CONTACT:-1}"
fi
if [[ ${SKIP_LEVEL:-0} == 1 && $mode != trace && $mode != fields-clear ]]; then script=profile_clear.gdb; fi
# Always leave the playable binary behind, including after a timeout/failure.
restore() {
  unset PROBES FPSCOUNT
  unset PROFILE_ENTRY
  unset PROFILE_SHOP
  unset PROFILE_OVERFLOW
  unset OVERFLOW_VERIFY
  unset PROFILE_NAVIGATE
  unset REFERENCE_RANDOM
  unset REFERENCE_BOX_RANDOM
  unset REFERENCE_PATTERNS
  unset REFERENCE_BREAKABLES
  unset REFERENCE_DAMAGE_TEXT
  unset REFERENCE_DAMAGE_BANK
  unset REFERENCE_HEALTH_ART
  unset REFERENCE_HEALTH_ADMISSION
  unset REFERENCE_AMMO_ADMISSION
  unset REFERENCE_GEM_POSITION
  unset REFERENCE_EFFECT_COMPOSITION
  unset REFERENCE_STYLE_SPRITES
  unset REFERENCE_SELECTION_BINDINGS
  unset REFERENCE_CLEAR_MOTION
  unset REFERENCE_ACTOR_PREFIX
  unset REFERENCE_DRAW_LIST
  unset REFERENCE_VALIDATION
  unset REFERENCE_ANIMATION
  unset REFERENCE_GENERATED_BOXES
  unset REFERENCE_INITIAL_RASTER
  unset REFERENCE_INITIAL_RASTER_STORAGE
  unset REFERENCE_ROOM_PREPARE
  unset REFERENCE_FIELD_BANK
  unset REFERENCE_ENTRY_ASSETS
  unset REFERENCE_EMPTY_TERRAIN
  unset REFERENCE_ROOM_INDEX
  unset REFERENCE_ROOM_TERRAIN
  unset REFERENCE_ROOM_VIEWS
  unset REFERENCE_INITIAL_TERRAIN
  unset REFERENCE_BOX_ALARMS
  unset REFERENCE_HUD_TEXT
  unset REFERENCE_HUD_WORDS
  unset REFERENCE_HUD_BORDER
  unset REFERENCE_CONTACT_MOTION
  unset REFERENCE_CONTACT_COORDINATES
  unset REFERENCE_ENEMY_ALARM_ROSTER
  unset DRAW_CACHE_AUDIT
  unset DRAW_ORDER_AUDIT
  unset LOCAL_DRAW_CAPTURE
  unset AMMO_FACTORED
  unset AMMO_FACTORED_VERIFY
  unset PREPARED_FIELDS_VERIFY PROFILE_FIELD_REFERENCE PREPARED_FIELDS_BACKING
  unset TABLE_CONTACT_ATAN RETAIN_HEALTH_HUD AMMO_DAMAGE_RANGE PREPARED_AMMO_DIVIDERS PREPARED_FIELDS
  unset SCENERY_LAYERS NATIVE_BOXES NATIVE_FIELDS TERRAIN_BATCH TERRAIN_DIRECT
  make clean > .run/profile-restore.log 2>&1
  make -j4 TERRAIN_BATCH_VERIFY=0 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_LASER=0 PROFILE_CONTACT=0 PROFILE_FRAMES=0 PROFILE_TRANSITION=0 GENERATION_DETAIL=0 GENERATION_TIMING=0 PHASE_SAMPLING=0 TRACE_FRAMES= >> .run/profile-restore.log 2>&1
}
mkdir -p .run
if [[ ${PROFILE_OVERFLOW:-0} == 1 ]]; then
  make -C .. test-amiga-currency-promotion
fi
if [[ $mode == overflow-state ]]; then make -C .. generate-overflow-witness; fi
if [[ ${PROFILE_SHOP:-0} == 1 ]]; then
  make -C .. generate-shop-profile
fi
if [[ ${PROFILE_ENTRY:-0} == 1 ]]; then
  make -C .. verify-entry-profile-input
fi
if [[ ${PROFILE_CONTACT:-0} == 1 ]]; then
  env -u DOWNWELL_CONTACT_PROFILE_SEARCH -u DOWNWELL_ENCOUNTER_PROFILE_SEARCH \
    -u DOWNWELL_ENCOUNTER_SEEDS -u DOWNWELL_ENCOUNTER_CASE make -C .. verify-contact-profile
fi
trap restore EXIT
make clean
make -j4 "${flags[@]}" PROFILE_AUTORUN=1 PROFILE_TRANSITION="$transition"
printf 'Profile optimization: %s\n' "${AMIGA_OPTIMIZATION:--O3}"
printf 'Profile prepared field reference: %s\n' "${PROFILE_FIELD_REFERENCE:-0}"
printf 'Profile Laser Sight fixture: %s\n' "${PROFILE_LASER:-0}"
printf 'Profile natural contact tape: %s\n' "${PROFILE_CONTACT:-0}"
printf 'Profile natural overflow tape (seed 8, route 4): %s\n' "${PROFILE_OVERFLOW:-0}"
printf 'Profile inner generation probes: %s (only active in probes mode)\n' "${GENERATION_DETAIL:-0}"
printf 'Profile coarse generation timing: %s (FPS mode only)\n' "${GENERATION_TIMING:-0}"
printf 'Profile coarse timing stage: %s\n' "${GENERATION_TIMING_PHASE:-profileGeneration}"
shasum -a 256 out/Downwell.exe
if [[ ${PREPARED_FIELDS:-0} == 1 ]]; then
  m68k-amiga-elf-objdump -s -j .text out/Downwell.elf |
    sed -n '/^ [0-9a-f][0-9a-f]* /p' > .run/profile-text.dump
  shasum -a 256 .run/profile-text.dump
fi
VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-60}" \
GDB_SOURCE="$script" READY_MARKER=DOWNWELL_PROFILE_READY \
READY_DESCRIPTION="bounded scene profile" bash verify_startup.sh
if [[ ${PROFILE_ENTRY:-0} == 1 ]]; then
  cmp .run/entry-hud.bin ../generated/amiga-hud.dwap
  cmp .run/entry-fields.bin ../generated/amiga-field-images.bin
  cmp .run/entry-results.bin ../generated/amiga-results.dwap
fi
cp .run/gdb-startup.log ".run/profile-${mode}.log"
if [[ ${GENERATION_TIMING:-0} == 1 ]] && ! grep -q '^PHASE coarse:' ".run/profile-${mode}.log"; then
  echo "Requested coarse generation measurement was not produced" >&2
  exit 1
fi
if [[ ${PHASE_SAMPLING:-0} == 1 ]] && ! grep -q '^SAMPLE total=' ".run/profile-${mode}.log"; then
  echo "Requested phase samples were not produced" >&2
  exit 1
fi
if [[ $mode == frames ]]; then
  for bin in "$dump_out"/frame-*.bin; do python3 ../tools/decode-amiga-gameplay-frame.py "$bin" "${bin%.bin}.png"; done
  echo "Frames: $dump_out"
  exit 0
fi
if [[ $mode == trace ]]; then
  ls "$trace_out"/trace-*.bin > /dev/null 2>&1 || { echo "FS-UAE wrote no trace" >&2; exit 1; }
  cp out/Downwell.elf "$trace_out/Downwell.elf"
  cp ".run/profile-${mode}.log" "$trace_out/capture.log"
  grep '^TRACE' "$trace_out/capture.log"
  echo "Traces: $trace_out (report: python3 tools/amiga-trace-report.py $trace_out)"
  exit 0
fi
if [[ $mode == draw-cache ]]; then
  grep '^DRAW_CACHE' ".run/profile-${mode}.log"
elif [[ $mode == draw-order ]]; then
  grep '^DRAW_ORDER' ".run/profile-${mode}.log"
elif [[ $mode == ammo ]]; then
  grep '^AMMO' ".run/profile-${mode}.log"
elif [[ $mode == shop || $mode == terrain-shop ]]; then
  if [[ $mode == shop ]]; then
    cmp ../generated/amiga-hud.dwap .run/shop-profile-hud.bin
    echo "SHOP_HUD resident bank matches offline source byte-for-byte"
  fi
  grep '^SHOP_' ".run/profile-${mode}.log"
else
  sed -n '/PROFILE/,$p' ".run/profile-${mode}.log"
fi
