# Logical publication and derived-tree scheduling

`GameplayCollisionOwner` owns more than collision acceleration: lifetime keys,
published positions and bounds, object registration, animation snapshots, layer
membership and destruction ordering are live simulation state. None of those
may be delayed merely because a frame does not query the spatial tree.

`RunnerSpatialJournal` schedules only the derived `RunnerSpatialIndex`. It
records each admitted insertion/removal in order. The bounds-publication caller
still updates cached bounds, flags and dirty queues at the original execution
point. Equal updates are not coalesced. A removed/reused slot retains its two
separate operations; rebuilding from the currently live objects would lose
observable tree traversal order.

Dirty-queue links use 16-bit internal indices when the capacity/end sentinel
fits, and 32-bit indices otherwise. A queued slot is either the head or has a
predecessor; removal resets both links. No separate membership flag is needed.
This changes neither remove-and-prepend ordering nor setter admission: resource
IDs and generation counters remain full width, and dirty queues are still
updated synchronously, independently of the derived-tree journal.

## Observation boundaries

Box queries, line queries, tree height and node count first apply every pending
operation. This includes height reads that select the list/tree path before
family preparation. Source query preparation, dirty flushing, temporary caller
movement and restoration remain in `RunnerSpatialInstances`, unchanged. See
[query mutation](time-stop-collision.md#query-mutation-contract).

Between observations, `service(budget)` applies at most that many complete
operations. It does not run Create, change RNG, modify the registration list,
dispatch an event or touch display memory. The eager index remains the portable
reference and the fallback implementation.

## Admission and failure timing

Deferral requires unique bounded slot IDs, integral non-inverted rectangles,
no negative zero, and historical union extents at most 65535 with area at most
2^24. Every rectangle/union then has exact finite binary32 area arithmetic;
deferred replay cannot introduce a numeric failure. The historical union is
conservative and does not shrink after retirement.

Pool admission is conservative too. A tree has at most N leaves and N-1
internal nodes for N entries. Removal can retain one underfull detached node
per level while reinserting; the journal reserves two full depth allowances
plus the empty root, in addition to 2N nodes. Near that bound it catches up and
switches permanently to eager operation until `clear()`. It does not postpone
an allocation failure until a later query.

Unique-identity removal with full-root retry has an immediate known result.
An absent identity is a tree no-op. For a present identity the log retains the
old bounds and replays the actual removal, including its condensation/reinsert
order. Duplicate insertions, unbounded IDs, no-retry removal, inadmissible
geometry and invalid inputs first catch up and use the original eager API.
The caller's failure result/output semantics remain those of that API.

Validity includes both the journal and its private tree. Eager mutation
fallbacks can poison the tree without setting the journal's own failure flag;
replay failure sets the latter. Rejected query arguments do not poison an
otherwise valid tree. Tests distinguish terminal mutation failure (including
subsequent operation refusal) from recoverable query rejection and check that
`clear()` resets both owners. The outer instance owner's height reads can
themselves trigger journal replay, so index validity is not merely an
immutable precondition of those calls.

The gameplay owner's log contains 4096 records (the generic journal's default
remains 512 for reference users). Pressure drains it synchronously rather than
dropping operations or inventing a queue-full game failure. The larger log adds
78,848 bytes over the 512-record 68000 layout (22 bytes per operation), in
ordinary program storage, not Chip RAM. This capacity accommodates a level's
2048 terrain slots plus churn, but is not an unbounded lifetime history.
Both pressure drains
and observation catch-up can be expensive: deferral alone is not a frame-budget
guarantee.

## Backend scheduling

The Amiga does not service the journal during its vertical-blank wait. Reads,
queue pressure and eager fallback are its materialization boundaries. This
avoids running an unneeded tree operation across VBI, but reads then bear the
catch-up cost. The explicit service API remains available for reference tests.
Profiling must cover queue pressure, query catch-up and natural query-bearing
gameplay, not just a selector-free generation tape. Removing idle service alone
does not establish bounded first-query latency or a 20 ms frame budget.

Tests compare exact box/line hit order, first-hit stops, height and node counts
against eager execution while varying partial service and buffer pressure.
They also cover retirement/reuse, full-root retry, capacity failure, clear and
fallbacks for fractional bounds, negative zero, duplicate IDs and invalid input.
These are implementation-equivalence checks; they do not establish that the
port has recovered every original actor, event or collision consumer.
