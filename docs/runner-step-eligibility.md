# Creation during a Step phase

Read-only inspection of the pinned Windows `Downwell.exe` establishes a
phase-local creation cutoff, not a whole-frame exclusion rule.

- Instance initialization at `0x461778` loads the current 64-bit counter from
  `0x9360a0`/`0x9360a4` into instance offsets `0x60`/`0x64`.
- The Step dispatcher `0x50b7e0` saves the old counter at `0x50b813` and
  increments the global counter before traversing event-bearing objects.
- At `0x50b8ab` it filters instance flags with `0x100003`. At `0x50b8b6`
  it compares the instance creation timestamp against the saved cutoff;
  a larger timestamp skips dispatch. Eligible instances are passed as self
  and other to `0x444910`, event type 3 and the requested subtype.
- The frame loop calls this dispatcher with subtype 0 (ordinary Step) at
  `0x50dd76`, then with subtype 2 (End Step) at `0x50def3`. Begin Step uses
  subtype 1 earlier at `0x50dc7f`.

Therefore a bone created by skeleton ordinary Step does not run its own
ordinary Step in that same phase, even if traversal reaches its object list
later. A subsequent Step phase takes a new cutoff; the creation timestamp
does not itself exclude the child from that later phase. This does not prove
collision traversal, draw eligibility, activation flags, or ordering among
instances of the same object. Those remain separate integration questions.

`BoneProjectileStore::beginPhase()` captures a monotonic handle cutoff before
any actor executes that phase. `visitPhase()` excludes newer handles and
retired instances, including a newly created child occupying a not-yet-visited
slot. The world must capture before skeleton Step, not just before visiting
bones, and must capture anew for a later phase. Store slot order is not claimed
to reproduce every original instance-list ordering.

Normal and sanitizer tests cover creation after the cutoff, eligibility at
the next cutoff, retirement/recreation of a later slot during iteration and
the existing 2,048 owned creation/RNG cases. These remain component tests;
the gameplay world has not yet adopted the store.
