#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
mode="${1:-probes}"
transition=0
autorun=1
ready_marker=DOWNWELL_PROFILE_READY
case "$mode" in
  probes) flags=(PROBES=1); script=profile.gdb ;;
  fps) flags=(FPSCOUNT=1); script=profile_fps.gdb ;;
  idle)
    flags=(FPSCOUNT=1); script=profile_fps.gdb
    export PROFILE_IDLE=1 SKIP_TO_GAME=1 SKIP_INTRO=0 SKIP_LEVEL=0
    export PROFILE_CONTACT=0 PROFILE_SHOP=0 PROFILE_ENTRY=0 PROFILE_LASER=0 PROFILE_OVERFLOW=0 PROFILE_NAVIGATE=0
    ;;
  quiet-fall|quiet-state|quiet-window|terrain-reads)
    flags=(PROBES=0 FPSCOUNT=1 TRACE_FRAMES=165); script=profile_quiet_cadence.gdb
    ready_marker=DOWNWELL_QUIET_CADENCE_READY
    export DETERMINISTIC=1
    export STRIP=1 PROFILE_IDLE=1 PROFILE_FRAMES=600 SKIP_TO_GAME=1 SKIP_INTRO=0 SKIP_LEVEL=0
    export PROFILE_CONTACT=0 PROFILE_SHOP=0 PROFILE_ENTRY=0 PROFILE_LASER=0 PROFILE_OVERFLOW=0 PROFILE_NAVIGATE=0
    # Every mode runs on MACHINE (default a1200-4m, the target); MACHINE=a500
    # remains for relative comparisons with the A500 evidence.
    export VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-90}"
    if [[ $mode == quiet-state ]]; then
      # Stops at every update: state equivalence only, never timing evidence.
      script=profile_quiet_landing.gdb
      ready_marker=DOWNWELL_QUIET_LANDED
      if [[ ${TERRAIN_BATCH_VERIFY:-0} == 1 ]]; then
        mkdir -p ../tmp
        sed -e 's/^set $sPatternVerification = 0$/set $sPatternVerification = 1/' \
          profile_quiet_landing.gdb > ../tmp/profile-quiet-pattern-rows.gdb
        script="$PWD/../tmp/profile-quiet-pattern-rows.gdb"
      fi
    fi
    if [[ $mode == terrain-reads ]]; then
      flags+=(TERRAIN_READ_CENSUS=1)
      script=profile_terrain_reads.gdb
      ready_marker=DOWNWELL_TERRAIN_READS_READY
    fi
    if [[ $mode == quiet-window ]]; then
      # Same binary and trajectory as quiet-fall. The trace starts one mark
      # before the measured interval; its first/last partial frames are excluded.
      mkdir -p ../tmp/trace
      trace_out=$(mktemp -d "$PWD/../tmp/trace/quiet-window.XXXXXX")
      sed "s|@OUTPUT@|$trace_out|g" profile_quiet_window.gdb > "$trace_out/profile_quiet_window.gdb"
      script="$trace_out/profile_quiet_window.gdb"
      ready_marker=DOWNWELL_PROFILE_READY
    fi
    ;;
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
        if [[ ${DUMP_DETAILS:-0} == 1 ]]; then
          printf 'dump binary memory %s/bobs-%s.bin gDownwellDumpBobs gDownwellDumpBobs+gDownwellDumpBobCount\n' "$dump_out" "$tick"
          printf 'dump binary memory %s/pack-%s.dwap gDownwellDumpPack gDownwellDumpPack+gDownwellDumpPackBytes\n' "$dump_out" "$tick"
        fi
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
  route)
    # W3.6: CENSUS=1 ROUTE=name replays the host route tmp/sound-route-name
    # (make test-sound-routes) from boot with joystick events only
    # (tools/verify-world-one-native.py, its witness and sound checks), on an
    # FPSCOUNT build without autorun. hard-mode-death* routes start from a
    # save with hard mode on and end at the results.
    # ROUTE_END_TICK=T ends the route at simulation tick T (a short census).
    # ROUTE_TRACE=N (W3.7, without CENSUS) records ROUTE_TRACE_FIELDS
    # (default 20) whole fields of every instruction from the input sample
    # of gameplay update N-1, into TRACE_OUTPUT, and ends there.
    route="${ROUTE:-world-one}"
    [[ -f ../tmp/sound-route-$route.inputs ]] || {
      echo "no host route tmp/sound-route-$route (make test-sound-routes)" >&2; exit 1; }
    flags=(FPSCOUNT=1); autorun=0; script=verify_startup.gdb
    ready_marker=DOWNWELL_WORLD_ONE_READY
    export GDB=../tools/verify-world-one-native.py DOWNWELL_ROUTE_TAPE="../tmp/sound-route-$route"
    export DOWNWELL_ROUTE_CENSUS=1 SKIP_INTRO=0 SKIP_TO_GAME=0 SKIP_LEVEL=0 PROFILE_FRAMES=0
    export PROFILE_CONTACT=0 PROFILE_SHOP=0 PROFILE_ENTRY=0 PROFILE_LASER=0 PROFILE_OVERFLOW=0 PROFILE_NAVIGATE=0 PROFILE_IDLE=0
    export VERIFY_TIMEOUT_SECONDS="${VERIFY_TIMEOUT_SECONDS:-3000}"
    export DOWNWELL_ROUTE_SECONDS="$VERIFY_TIMEOUT_SECONDS"
    if [[ $route == hard-mode-death* ]]; then
      mkdir -p .run
      printf '[stats]\nhardUnlocked="1.000000"\n[option]\nhardMode="1.000000"\n' > .run/route-save.ini
      export DOWNWELL_ROUTE_DEATH=1 VERIFY_SAVE="$PWD/.run/route-save.ini"
      ready_marker=DOWNWELL_WORLD_ONE_RESULTS_READY
    fi
    if [[ -n ${ROUTE_END_TICK:-} ]]; then
      [[ $ROUTE_END_TICK =~ ^[1-9][0-9]*$ ]] || { echo "ROUTE_END_TICK must be a tick" >&2; exit 1; }
      export DOWNWELL_CAPTURE_TICK="$ROUTE_END_TICK"
      ready_marker=DOWNWELL_WORLD_ONE_CAPTURE_READY
    fi
    if [[ -n ${ROUTE_TRACE:-} ]]; then
      [[ $ROUTE_TRACE =~ ^[1-9][0-9]*$ && ${ROUTE_TRACE_FIELDS:-20} =~ ^[1-9][0-9]*$ ]] || {
        echo "ROUTE_TRACE must be a gameplay update and ROUTE_TRACE_FIELDS a field count" >&2; exit 1; }
      (( ${ROUTE_TRACE_FIELDS:-20} <= 100 )) || { echo "ROUTE_TRACE_FIELDS must not exceed 100" >&2; exit 1; }
      trace_out="${TRACE_OUTPUT:-$PWD/../tmp/trace/route-$route-$ROUTE_TRACE}"
      mkdir -p "$trace_out"
      trace_out="$(cd "$trace_out" && pwd)"
      rm -f "$trace_out"/trace-*.bin
      flags+=(TRACE_FRAMES="$ROUTE_TRACE")
      export DOWNWELL_ROUTE_TRACE="$ROUTE_TRACE" DOWNWELL_ROUTE_TRACE_FIELDS="${ROUTE_TRACE_FIELDS:-20}"
      export DOWNWELL_ROUTE_TRACE_OUTPUT="$trace_out"
      ready_marker=DOWNWELL_WORLD_ONE_TRACE_READY
    fi
    ;;
  *) echo "usage: [CENSUS=1] bash amiga/profile.sh [probes|fps|idle|quiet-fall|quiet-state|quiet-window|terrain-reads|overflow-state|shop|terrain-shop|trace|frames|draw-cache|draw-order|ammo|transition|entry|terrain|fields|fields-clear|route]" >&2; exit 1 ;;
esac
if [[ $mode == draw-cache || $mode == draw-order || $mode == ammo ]]; then
  export SKIP_TO_GAME=1 SKIP_LEVEL=0 PROFILE_CONTACT="${PROFILE_CONTACT:-1}"
fi
if [[ ${SKIP_LEVEL:-0} == 1 && $mode != trace && $mode != fields-clear ]]; then script=profile_clear.gdb; fi
# W3.5: CENSUS=1 adds the per-update work census to a mode that runs to the
# profile freeze without probes or traces. The debugger reads its ring once,
# just before the mode's ready marker. A debugger stop halts the whole
# emulated machine, beam included: 165 stops inside the quiet fall left the
# ring byte-identical, so the route driver's stops at every input sample
# add nothing to the measured updates.
census="${CENSUS:-0}"
unset CENSUS
if [[ $census == 1 ]]; then
  case "$mode" in
    fps|idle|quiet-fall|shop|overflow-state|draw-cache|draw-order|ammo) ;;
    # Their routes, without the probes: census.gdb only waits for the freeze.
    entry|transition) flags=(FPSCOUNT=1); script=census.gdb; ready_marker=DOWNWELL_PROFILE_READY ;;
    # The route driver reads the ring itself where the route ends.
    route) ;;
    *) echo "CENSUS=1 needs a mode that runs to the profile freeze or a route, without probes or traces" >&2
       exit 1 ;;
  esac
  [[ " ${flags[*]} " != *" PROBES=1 "* ]] || { echo "CENSUS=1 cannot measure a probes build" >&2; exit 1; }
  flags+=(CENSUS=1 CENSUS_UPDATES="${CENSUS_UPDATES:-16384}")
  if [[ $script != census.gdb && $mode != route ]]; then
    mkdir -p .run/census
    awk '/^shell touch \.run\/gdb-ready\.marker$/ && !done {print "source census_dump.gdb"; done=1} {print}' \
      "$script" > .run/census/profile.gdb
    grep -qx 'source census_dump.gdb' .run/census/profile.gdb || {
      echo "$script has no ready marker to read the census at" >&2; exit 1; }
    script=.run/census/profile.gdb
  fi
  rm -f .run/census.bin .run/blit-census.bin
elif [[ $mode == route && -z ${ROUTE_TRACE:-} ]]; then
  echo "route measures nothing without CENSUS=1 or ROUTE_TRACE (tools/verify-world-one-native.py replays it)" >&2
  exit 1
fi
if [[ $mode == route && -n ${ROUTE_TRACE:-} && $census == 1 ]]; then
  echo "ROUTE_TRACE traces a build without the census" >&2; exit 1
fi
# Always leave the playable binary behind, including after a timeout/failure.
restore() {
  unset SCENE_VERIFY
  unset CENSUS CENSUS_UPDATES
  unset TERRAIN_READ_CENSUS
  unset PROBES FPSCOUNT
  unset PROFILE_ENTRY
  unset PROFILE_SHOP
  unset PROFILE_OVERFLOW
  unset OVERFLOW_VERIFY
  unset PROFILE_NAVIGATE
  unset PROFILE_IDLE
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
  unset REFERENCE_SHADE_PARTITION
  unset REFERENCE_CURRENCY_BOBS
  unset REFERENCE_STYLE_SPRITES
  unset REFERENCE_SELECTION_BINDINGS
  unset REFERENCE_SELECTION_BORDER_OWNERSHIP
  unset REFERENCE_SELECTION_STRIPS
  unset REFERENCE_CLEAR_OPTION_HEIGHT
  unset REFERENCE_CLEAR_HEADINGS
  unset REFERENCE_CLEAR_MOTION
  unset REFERENCE_ACTOR_PREFIX
  unset REFERENCE_DRAW_LIST
  unset REFERENCE_VALIDATION
  unset REFERENCE_ANIMATION
  unset REFERENCE_GENERATED_BOXES
  unset REFERENCE_INITIAL_RASTER
  unset REFERENCE_INITIAL_RASTER_STORAGE
  unset REFERENCE_ROOM_PREPARE
  unset REFERENCE_TERRAIN_RECORDS
  unset REFERENCE_FIELD_BANK
  unset REFERENCE_FIELD_ADMISSION
  unset REFERENCE_ENTRY_ASSETS
  unset REFERENCE_EMPTY_TERRAIN
  unset REFERENCE_ROOM_INDEX
  unset REFERENCE_ROOM_TERRAIN
  unset REFERENCE_ROOM_VIEWS
  unset REFERENCE_INITIAL_TERRAIN
  unset REFERENCE_BOX_ALARMS
  unset REFERENCE_HUD_TEXT
  unset REFERENCE_HUD_WORDS
  unset REFERENCE_HUD_COMPOSITION
  unset REFERENCE_HUD_BORDER
  unset REFERENCE_HUD_BORDER_EXTENT
  unset REFERENCE_CONTACT_MOTION
  unset REFERENCE_CONTACT_COORDINATES
  unset REFERENCE_CELL_REPAIR CELL_REPAIR_VERIFY EFFECT_BLIT_CENSUS BLIT_CENSUS CENSUS_PARTS EFFECT_CLEAR_VERIFY EFFECT_PIXEL_VERIFY
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
make -j4 "${flags[@]}" PROFILE_AUTORUN="$autorun" PROFILE_TRANSITION="$transition"
printf 'Profile optimization: %s\n' "${AMIGA_OPTIMIZATION:--O3}"
printf 'Profile prepared field reference: %s\n' "${PROFILE_FIELD_REFERENCE:-0}"
printf 'Profile Laser Sight fixture: %s\n' "${PROFILE_LASER:-0}"
printf 'Profile natural contact tape: %s\n' "${PROFILE_CONTACT:-0}"
printf 'Profile no-input gameplay tape: %s\n' "${PROFILE_IDLE:-0}"
printf 'Profile natural overflow tape (AmigaOverflowProfileEntry.h): %s\n' "${PROFILE_OVERFLOW:-0}"
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
GDB_SOURCE="$script" READY_MARKER="$ready_marker" \
READY_DESCRIPTION="bounded scene profile" bash verify_startup.sh
if [[ ${PROFILE_ENTRY:-0} == 1 && $census != 1 ]]; then
  cmp .run/entry-hud.bin ../generated/amiga-hud.dwap
  cmp .run/entry-fields.bin ../generated/amiga-field-images.bin
fi
cp .run/gdb-startup.log ".run/profile-${mode}.log"
if [[ $census == 1 ]]; then
  census_out="${CENSUS_OUTPUT:-$PWD/../tmp/census/$mode${route:+-$route}}"
  mkdir -p "$census_out"
  cp .run/census.bin "$census_out/census.bin"
  cp ".run/profile-${mode}.log" "$census_out/capture.log"
  cp out/Downwell.elf "$census_out/Downwell.elf"
  [[ ${BLIT_CENSUS:-0} != 1 ]] || cp .run/blit-census.bin "$census_out/blit-census.bin"
  CENSUS_PARTS_FILE=
  if [[ ${CENSUS_PARTS:-0} == 1 ]]; then
    cp .run/census-parts.bin "$census_out/census-parts.bin"; CENSUS_PARTS_FILE="$census_out/census-parts.bin"
  fi
  python3 ../tools/amiga-census-report.py "$census_out/census.bin" "$census_out/capture.log" \
    --list=over --json="$census_out/census.json" \
    ${CENSUS_PARTS_FILE:+--parts="$CENSUS_PARTS_FILE"} | tee "$census_out/census.txt"
  echo "Census: $census_out"
fi
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
if [[ $mode == route && -n ${ROUTE_TRACE:-} ]]; then
  ls "$trace_out"/trace-*.bin > /dev/null 2>&1 || { echo "FS-UAE wrote no trace" >&2; exit 1; }
  cp out/Downwell.elf "$trace_out/Downwell.elf"
  cp ".run/profile-${mode}.log" "$trace_out/capture.log"
  grep '^TRACE' "$trace_out/capture.log"
  echo "Traces: $trace_out (report: python3 tools/amiga-trace-report.py $trace_out)"
  exit 0
fi
if [[ $mode == trace || $mode == quiet-window ]]; then
  ls "$trace_out"/trace-*.bin > /dev/null 2>&1 || { echo "FS-UAE wrote no trace" >&2; exit 1; }
  cp out/Downwell.elf "$trace_out/Downwell.elf"
  cp ".run/profile-${mode}.log" "$trace_out/capture.log"
  grep '^TRACE' "$trace_out/capture.log"
  if [[ $mode == quiet-window ]]; then
    python3 ../tools/amiga-trace-report.py "$trace_out" --frames=57-121 --require-complete \
      --frame-summary="$trace_out/frames.json" --causes > "$trace_out/quiet.txt"
    python3 ../tools/amiga-trace-report.py "$trace_out" --frames=122 --require-complete \
      --causes > "$trace_out/generation-122.txt"
    # Work excludes the publication wait and all its children (idle RNG service).
    python3 ../tools/amiga-trace-report.py "$trace_out" --frames=57-122 --require-complete \
      --work-summary="$trace_out/work.json" --work-wait=AmigaPlatform::waitPublished > /dev/null
    # Each summary meets the baseline of its machine's clock and kind, if any.
    python3 ../tools/verify-quiet-window.py "$trace_out/frames.json" "$trace_out/work.json" \
      "$trace_out/Downwell.elf"
    echo "Quiet updates 57–121: $trace_out/quiet.txt"
    echo "First generation burst 122: $trace_out/generation-122.txt"
    echo "Exact complete-frame cycles and build hash: $trace_out/frames.json"
    echo "Exact work of updates 57–122: $trace_out/work.json"
  fi
  echo "Traces: $trace_out (report: python3 tools/amiga-trace-report.py $trace_out)"
  exit 0
fi
if [[ $mode == terrain-reads ]]; then
  python3 ../tools/report-terrain-reads.py .run/terrain-read-census.bin
elif [[ $mode == quiet-state ]]; then
  grep '^QUIET ' ".run/profile-${mode}.log"
  [[ $(grep -c '^QUIET ' ".run/profile-${mode}.log") == 165 ]] || {
    echo "Expected all 165 initial-fall state samples" >&2; exit 1;
  }
  # The samples are machine-independent: the A1200 and A500 agree exactly.
  if [[ ${QUIET_STATE_RECORD:-0} == 1 ]]; then
    grep '^QUIET ' ".run/profile-${mode}.log" > ../docs/amiga-quiet-states.txt
    echo "Recorded docs/amiga-quiet-states.txt"
  else
    diff <(grep '^QUIET ' ".run/profile-${mode}.log") ../docs/amiga-quiet-states.txt || {
      echo "Initial-fall state samples differ from docs/amiga-quiet-states.txt" >&2; exit 1;
    }
    echo "All 165 initial-fall state samples match docs/amiga-quiet-states.txt"
  fi
elif [[ $mode == quiet-fall ]]; then
  # The landing witness is always checked. On the target every update of the
  # fall presents in one field (164 fields), so any missed field fails. The
  # A500's old 197 no longer holds (W2.6 measured 210), so other machines are
  # judged only with an explicit QUIET_MAX_FIELDS.
  . ./machine.sh
  quiet_target_ceiling=164
  quiet_ceiling=
  if [[ -z ${VERIFY_CHIP_KB:-} && ${MACHINE:-${VERIFY_MACHINE:-$DOWNWELL_DEFAULT_MACHINE}} == a1200-4m ]]; then
    quiet_ceiling=$quiet_target_ceiling
  fi
  quiet_ceiling="${QUIET_MAX_FIELDS:-$quiet_ceiling}"
  python3 ../tools/verify-quiet-fall.py ".run/profile-${mode}.log" \
    ${quiet_ceiling:+--max-fields "$quiet_ceiling"}
  grep '^QUIET_' ".run/profile-${mode}.log"
elif [[ $mode == draw-cache ]]; then
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
