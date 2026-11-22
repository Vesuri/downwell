#!/bin/bash
# WR.5 gate: replay every recorded slice route with and without the collision
# owner and encounter session (build/amiga-world-one-replay-test) and require
# the same witnesses, sound requests, gameplay draw lists and effect-surface
# producers. Run from the repository root after `make test-sound-routes`.
set -u
out=tmp/ownership
mkdir -p "$out"
packs="generated/downwell-rooms.dwrm generated/downwell-objects.dwob generated/downwell-builder-patterns.dwbp"
fail=0
for route in world-one world-one-v106-tour hard-mode-death hard-mode-death-v80-tour hard-mode-death-v195 \
    hard-mode-death-v514 hard-mode-death-v294-tour hard-mode-death-v325-tour \
    upgrade-0-18-v58 upgrade-5-9-v96 upgrade-3-v58 upgrade-11-v18 upgrade-8-v17 upgrade-15-v2 upgrade-1-v682-shot upgrade-17-v16 upgrade-22-v25 upgrade-12-v155; do
  hard=; case $route in hard*) hard=1;; esac
  for mode in owned unowned; do
    unowned=; [ $mode = unowned ] && unowned=1
    env ${hard:+DOWNWELL_SLICE_HARD=1} ${unowned:+DOWNWELL_REPLAY_UNOWNED=1} \
      DOWNWELL_REPLAY_DRAWS=$out/$route.$mode.draws DOWNWELL_REPLAY_SOUNDS=$out/$route.$mode.sounds \
      ./build/amiga-world-one-replay-test $packs tmp/sound-route-$route > $out/$route.$mode.log 2>&1 &
  done
  wait
  for mode in owned unowned; do
    tail -1 $out/$route.$mode.log | grep -q '^REPLAY' ||
      { echo "$route $mode: $(tail -1 $out/$route.$mode.log)"; fail=1; }
    # Registry presence (T, R: -1 without an owner) and the owner's own
    # traversal (after |) are diagnostics, not drawn output.
    sed -e 's/ |.*//' -e 's/ T-\{0,1\}[0-9]*//' -e 's/ R[-0-9/]*//' $out/$route.$mode.draws > $out/$route.$mode.d
  done
  cmp -s $out/$route.owned.sounds $out/$route.unowned.sounds && sounds=same || { sounds=different; fail=1; }
  if cmp -s $out/$route.owned.d $out/$route.unowned.d; then draws=same
  else draws="different from line $(cmp $out/$route.owned.d $out/$route.unowned.d | sed 's/.*line //')"; fail=1; fi
  echo "$route: sounds $sounds, draws $draws"
done
exit $fail
