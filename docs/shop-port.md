# Shop implementation notes

Source: local UTMT 0.9.2.0 `Assembly/gml_Object_Shop_Create_0.asm` stock/price
branches and `Assembly/gml_Object_Shop_Step_0.asm` blocks 74–80. Consumable base
prices are in `scrUpgrades` (IDs 100–105). Decompiled views were cross-checked
against VM order for randomness, price calculation and payment.

`ShopInventory` preserves these rules:

- Slot 1 rolls 100–101 even when health <=2 subsequently forces item 100.
- Slot 2 rerolls 100–103 until distinct from slot 1.
- Slot 3 rolls 102–105 once, discards that result, then rerolls until distinct
  from the first two. This otherwise redundant roll advances the game RNG.
- Price adds 200 per area after area one, doubles in hard mode, applies style-4
  discount, then membership discount to the already-discounted price. Both use
  ties-to-even `round(price/10)`; combining the percentages changes prices.
- The one-based item cursor checks stock before funds, decrements stock, calls
  the upgrade grant synchronously, then subtracts price. The grant can add
  currency. Spending alone does not change total collected gems or streak state.

The injected grant must implement `scrGainUg`, not immediately heal or refill
for IDs 100–105: those counts are consumed later by `scrControlBeginstepCheck`,
one unit per item type per update in increasing ID order. Curry increases maximum
health before `gainHp(1)`. At ownership cap, `scrGainUg` refunds 100 currency.
`Consumables` now implements that deferred grant/consumption path in `RunSession`.
Both runtimes call it at the controller boundary before player movement. Six
independent ID-ordered checks consume at most one of each item per update; they
are outside the source's pause/time-stop guard. Batteries refill current charge
after increasing capacity. Energy Drink heals before increasing charge. Curry
raises maximum health, then heals only one point (not to full health). Existing
`HealthReserve` preserves overflow healing and heart-piece conversion.

The queue resets with the run. It exposes ordered notification requests and
healing/max-health message requests without drawing or playing sounds; those
presentation consumers remain open. It is not a live shop menu: shop entry,
purchase input, case destruction and merchant feedback still need integration.

The range callback must use the shared Windows-runner RNG. This module covers
only the stock-selection segment, not random effects from other objects created
earlier/later during Shop Create. A full shop replay must preserve those too.

`make test-shop-inventory` verifies fixed random-call sequences, duplicate
rejection, the low-health override, all six prices over areas 1–5 and discount
combinations, callback/payment ordering, sold-out precedence, exact-price and
failed purchases, unchanged collection totals, and fresh stock on recreation.
The focused tests pass. Instantiating both paths with the native 68000 compiler
succeeds; undefined arithmetic helpers are only 32-bit unsigned divide/modulo.
No emulator run is claimed for this unconnected module, and neither playable
executable is changed by this initial stock/transaction implementation.

`make test-consumables` checks all six effects together in source order, stacked
items over successive updates, notifications, injured Curry behavior, and a
capped grant through the actual purchase transaction. The full host suite also
exercises queued batteries/Curry through both live runtime controllers, comparing
counts and resulting health/charge while requiring native HUD invalidation.
The native cache verifier includes an isolated three-update consumable fixture
on the 68000; it does not change the main gameplay tape or pretend to visit a shop.
Full host tests and that bounded direct-entry A500 verifier pass: three consumable
updates, 101 fresh terrain compositions and the task-stack guard. Native 68000
instruction/helper audits are clean. Notification art/audio are still requests,
not implemented presentation, and no end-to-end shop purchase is claimed.

## Interaction controller

`ShopInteraction` implements the interaction portion of Shop Step, including the
state-zero selector and its independent transition into the item-selection branch.
The caller supplies collision/interactable predicates, input **edges**, the shared
`choose(1,2,3)` RNG operation, upgrade grant and ordered feedback sink. Input is
mutable: opening/state-zero confirmation consumes confirm; item confirmation
suppresses cancel but leaves confirm set, as in the VM. Right then left are
independent branches, including a second navigation check when state zero enters
state one in the same update. Cursor zero is Leave; slots one through three use
the shared purchase transaction. Reentry does not regenerate sold stock.

Feedback signals preserve greeting/sold-out random draws, navigation, purchase,
poor response, goodbye and mask invalidation. Adapters must translate these into
merchant/case/text/audio changes; these are not implemented NPC animations.
The death/criminal tail disables interaction but does not itself clear noControl;
only the source exit block clears that flag. Do not introduce an early death
return: original interaction runs before that tail when not criminal. The normal
controller's interactable predicate still determines whether opening is allowed.
Draw-time cursor animation/decay, radio/view responses and world instance dispatch
remain separate integration work.

`make test-shop-interaction` covers eligibility, opening/confirmation consumption,
queued purchase with simultaneous cancel, sold-out RNG, poor response without
RNG, direction order/wrap, thanked exit, reentry, cancel, state-zero fall-through
and death/criminal ordering. It passes alongside inventory and consumable tests.
The callable template compiles for 68000 without floating-point/64-bit helpers.
No emulator or playable-shop coverage is claimed for this unconnected controller.

## World-hook audit and player input prerequisite

The builder's `$` marker emits object 277 (Shop). Its resource sprite 480 uses
an axis-aligned mask at x-10..x-1, y-1..y+8, not the whole 24x24 image. Shop Create
adds the table at x+64, three cases at tableX-19/0/+19 and y-9, an invisible wall
at x,y+8, keeper at x+18,y, and item/text children. Registering only a counter
and menu would omit collision and presentation objects. Child/inherited event
dispatch and RNG order still need integration review.

`scrLevelPat1` suppresses shops in ordinary area 1/level 1 but NOT style 4. The
current generator only implements the ordinary first-level setup; its default
must stay unchanged. Later levels or the special style need their source setup,
not an arbitrary shop inserted into normal 1-1.

`objPlayer_n` Begin Step computes interactable from a parentInteractable overlap,
zero horizontal speed, grounded state and the instance's interactOk. The existing
shared player lacked that up-button gate. It now accepts the retained interactable
state and suppresses the grounded press/jump branch only, as in
`scrUpButtonFunctions`; airborne held fire is a separate branch. World adapters
now supply this state for explicitly registered shops (see below); ordinary
generation still does not register the shop and its children.

The same audit found `controlsLocked` suppressed jumping/horizontal input but
not firing, empty-charge braking or release handling. `scrPlMovement` skips the
whole up-button handler under noControl. The shared player now does too, while
continuing gravity, alarm countdown and raw edge tracking. A release that occurs
while locked must neither shorten a jump nor be replayed after unlocking.
Focused player tests cover those cases, held-fire resumption, the interaction
press guard and the independently allowed airborne held-fire branch.
The full host suite and bounded direct-entry A500 600-update cache check pass;
68000 instruction/helper audits remain clean. That target tape is a regression
check, not a visit to a shop; focused shared-player tests exercise the new guards.

## Registered shop world integration

`RunSession::shops` owns up to eight registered counters, inventories and menus.
Registration reports exhaustion with a null pointer before consuming randomness.
Run reset clears instances, input history, interaction eligibility and the shop
control lock. End Step retires counters outside the source's strict
viewY-320/viewY+3000 bounds, without inventing an unlock on destruction.

Both runtime adapters expose `registerShop` for the currently supported default
style/normal-mode setup and use their existing shared RNG stream. Player movement
sees retained eligibility; the post-movement mask query refreshes it before menu
input. Raw input edges continue advancing while movement is locked. Opening faces
the counter, consumes confirmation and suppresses jumping; purchases queue the
existing controller-phase consumables. Directional edges navigate without moving
the player. The shared world supports criminal/mask invalidation, but the adapters
still supply criminal=false because that gameplay state is not implemented.

The host paired-runtime fixture explicitly registers a counter at the entrance
floor, compares randomized stock, opens it, checks held input and insufficient
funds, then uses a test-only free battery to verify deferred grant timing. Leave
and reentry retain sold stock. This is separate from the ordinary generation and
combat tape. `test-shop-world` also covers inclusive integer mask edges, movement
eligibility, capacity exhaustion, retirement and run reset.

Remaining integration: production creation and children, source-wide ordinary
Step dispatch (including multiple shops), fractional mask queries, cancellation
input mapping, merchant/case/audio feedback and rendering. Current runtime hooks
run the menu immediately after player movement and retain presentation requests;
they are not a claim that the full original Shop event has been implemented.

Validation: the full host suite (including both runtime fixtures) and normal SDL
build pass. The bounded SKIP_TO_GAME A500 600-update cache verifier passes with
clean 68000 instruction/helper audits. That emulator tape contains no shop;
registered-shop lifecycle evidence is currently from the host paired fixture.

`ShopFeedback` now consumes the ordered interaction effects in shared world
state. Buying exhausted stock clears that case's stored-item flag and removes
its item child, not the glass case. Keeper thankful/confused/goodbye mutations
and image-reset requests follow Shop Step; opening clears only thankful/goodbye.
Per-update rising-text identifiers and ordered sound requests retain original
resource IDs, audio priority and merchant/UI channels. Purchases request merchant
104 at priority 90 before UI 105 at priority 50. Held input produces no repeated requests.
The bounded sound queue reports overflow explicitly.

These requests are not audio playback, localized text rendering, or the keeper's
own alarm/animation controller. Focused tests cover all reply IDs, sound ordering,
stock disappearance, transient request reset and opening flag preservation. The
paired runtime purchase fixture compares both targets' child and feedback state.

## Static child geometry and player collision

Registered shops now contribute their original child rectangles to both player
solid queries and nearest-landing queries. These are resource masks, not artwork
extents: table 278/sprite 481 is x+34..x+93,y..y+7; the three cases 279/sprite 482
are centered at x+45/64/83,y-9 with offsets -9..8 on each axis; invisible wall
98/sprite 46 is x-8..x+7,y..y+15. Purchased cases stay solid. The counter itself
does not become a wall.

The invisible wall's End Step calls `scrOutofview` (strict viewY-160 and
viewY+3000), independently of the counter's -320 threshold. The table and cases
have no retirement Step in their parent chain and survive until room teardown.
Consequently retiring a counter does not recycle its occupied shared slot. The
eight-slot bound remains explicit; future room-transition integration must clear
room-owned shops rather than silently discard surviving child objects.

Tests check inclusive cropped mask bounds and gaps, nearest tops, shared-player
landings on wall/case, sold-case solidity, and separate lifetimes. The paired
runtime fixture approaches from outside the invisible wall and walks against it
after leaving the menu. Both retain the same fractional position while remaining
in the last unblocked integer query cell. This validates the current shared
integer-cell player adapter, not unresolved Windows fractional-query semantics.

Enemy/projectile queries, case damage/theft, dynamic child state, presentation
and production generation remain open. The new rectangles are player collision
hooks, not a complete implementation of those child objects. Empty shop worlds
return immediately from geometry queries, avoiding an eight-slot scan in normal
first-level gameplay. Native callback wrappers rebind each update so host-side
runtime copies cannot retain a pointer to the previous wrapper owner.

Validation: full host tests and SDL build pass, including the paired counter-wall
fixture. The bounded SKIP_TO_GAME 600-update A500 cache regression passes with
clean 68000 instruction/helper audits. No target-side visit to a shop is claimed;
the ordinary first-level emulator tape checks existing gameplay for regressions.

## Static fixture drawing

Both backends now consume `drawShopFixtures`: counter 480, table 481 and three
intact cases 482, frame zero, at recovered Shop Create positions. The counter
disappears on its retirement; surviving table/cases remain drawable. Purchases
do not change the case image. The helper emits creation order; final overlap
ordering with merchant/items and original equal-depth instances still needs
Windows presentation validation.

The game planar pack includes those three frames, converted offline by the
existing generator. SDL draws the shared stream before the player; the Amiga
terrain compositor draws it into cached static rows. Registration/counter
retirement advances a fixture revision and marks only intersecting art rows
dirty in the terrain cache, to be refreshed on subsequent composition. A purchase
does not invalidate furniture. This does not yet implement live generation or
an immediate visible-region refresh for dynamically registered on-screen shops.

Merchant animation, item-child drawing and active menu/text remain unfinished.
Normal 1-1 still contains no shop. Focused tests verify the static command stream,
resource IDs, coordinates and revision behavior; existing planar renderer tests
exercise the generated pack. No complete shop screenshot is claimed yet.

At the fixture-only milestone the generated game pack was 294020 bytes, SHA-256
`cf990f9f640477e3b14b6fb43d434634a1f3882b5cce1ec2ba092b95d3d912b8`;
copyrighted payload remains ignored. Full host tests, SDL build, offline pack
verification and the bounded direct-entry A500 600-update cache regression pass.
The resident Chip-art budget assertion and 68000 helper/instruction audits pass.
This target tape still exercises ordinary gameplay, not an on-screen shop.

## Cased items and purchase refresh

The shared drawing stream now includes surviving item children at case.x,
case.y+1. `objObtainableUg` Create explicitly selects image_index=1 with speed
zero; its Step assigns only sprite_index. All six consumable sprites have two
frames in the resource metadata. Their mappings are 100..105 ->
152,143,151,147,150,149; frame 1 is converted offline, not substituted with the
frame-0 icon used in the active menu. Buying the last unit removes that item's
draw command while retaining the case.

Each shop now has a cached-art revision separate from its furniture revision.
Only creation, removal of contents and counter retirement change it. On Amiga,
changed shops invalidate their intersecting rows in the terrain raster before
side-room staging can reuse them. Visible rows are recomposed once and copied
through the existing double-buffer dirty-row mechanism. Changed offscreen rows
are excluded from the ring's retained interval. This supersedes the earlier
fixture-only limitation where visible changes waited for another composition.
The refresh is a bounded 33-row shop-art span, not a whole-frame redraw.

A host test using the native planar renderer compares this partial-row purchase
refresh against fresh composition with the ring wrapping at row 320. It requires
an actual pixel change and checks that only the sold icon's 24x24 footprint
changes. Shared tests check command removal and the one-time revision increment.
The larger game pack is 295892 bytes, SHA-256
`16bf038f9c0a0caa7a345cff01ea735cfce37cea3827690d53c4a991ba40c530`.
Target-side on-screen purchase verification remains pending; the ordinary A500
regression tape does not visit a shop. Item collision/pickup outside a case,
case theft/destruction, child birth-phase ordering and final equal-depth overlap
validation also remain open.

## Merchant controller

Both registered-shop adapters now update a shared `ShopKeeper`. Its three alarm
values advance as a snapshot before menu/merchant Step: angry blinking displays
frame 1 for five updates, then resets frame 0 and draws an inclusive 20..60 wait
from the existing gameplay RNG. Alarm 2 clears reaction flags. Step preserves
thankful/goodbye/confused branch precedence, strict player-position comparisons,
the 60-update reaction holds, 240-update goodbye timer, and area-three half speed.
Image-reset feedback is consumed by the child state, not left as an unhandled
request. Merchant state survives counter retirement just like the child instance.

Animation uses the existing positive binary64 integer arithmetic primitive
(`HudReal`) with exact source constants .09/.4/.2 and no target floating point.
Focused tests compare 10000 idle updates and 1000 half-speed updates against
host binary64 arithmetic, as well as alarm boundaries, reactions and RNG draws.
The paired shop runtime fixture compares the complete animation phase, speed,
sprite and all three alarms after each input update.

The current adapter order is player movement, keeper alarms, shop interaction,
keeper Step, animation advancement. Whole-world event ordering, child birth
timing and engine animation-phase equivalence still need original-runner
validation. Criminal conversion is exposed as a replacement request, not a
spawned shopkeeper enemy; the runtime criminal state is still unsupported.
This milestone does not draw the merchant, consume its audio requests or dirty
the terrain cache for its animation. Graphics integration remains next.

Validation: full host regressions and the bounded SKIP_TO_GAME A500 600-update
cache check pass, with clean 68000 helper/instruction audits. The emulator tape
still contains no merchant; the paired runtime and focused controller fixtures
are the current evidence for the new behavior, not an original-game visual
sign-off. Empty shop worlds skip merchant scans.

## Merchant moving-object graphics

`drawShopKeepers` emits the child's current sprite/frame at counter.x+18,y.
SDL draws it after the shop fixtures and before the player. Amiga appends it to
the existing moving-actor list in the same order; the existing priority allocator
selects offline DMA parts or falls back to masked BOBs in the inactive display.
BOB restoration uses the pristine terrain backing. Merchant animation does not
advance the static-shop art revision or dirty terrain rows. Final original-game
equal-depth overlap validation and the area-three bubble overlay remain open.

All frames of sprites 485,486,487,496,497 are generated offline. The existing
lossless cropper removes transparent canvas padding and preserves anchors, so
the 48-pixel source canvases fit the existing 32-line DMA slots. Merchant events
never change image_xscale, so unused mirrored planar copies are omitted; both
DMA facings are still generated and checked against the pixel oracle.

The initial expanded pack used 280236 resident Chip bytes and booted with 13872
bytes free in the configured 512 KiB Chip/8 MiB Fast A500 emulator. Omitting unused mirrored planar
copies saved 5472 bytes and restored the existing <=278000-byte resident-art
assertion without raising it. The full generated pack is 309748 bytes, SHA-256
`70c88c0b2e6ab5d1bc8687f2bc698683ab6cbb0f873bb5882f4a37d7fdedfaa6`.
The renderer test now requires every merchant frame and checks 19894 offline
sprite oracle cases. These checks do not replace an on-screen target shop test;
normal 1-1 generation still does not produce a shop.

Validation: full host tests, offline asset verification, SDL build and bounded
SKIP_TO_GAME A500 cache regression pass. The final pack uses 274764 resident
Chip bytes, leaving 19344 bytes at startup in that configuration. The target
retains its task-stack safety margin; 68000 instruction/helper audits are clean.
The cache verifier now reports resident art and startup headroom on every run.

## Creation-time farewell RNG correction

Menu-text recovery exposed a previously missing creation-time RNG call:
`Shop Create` ends with `scrShopAssignText`, whose VM pushes shopBye2/1/0 and
calls choose(argc=3) to initialize logThank. Registration now draws inclusive
0..2 on the same stream after stock setup and retains that farewell variant.
It is not rerolled when the merchant thanks the player or on reentry. The
inventory module remains stock-only; this extra draw belongs to world creation.
Focused tests assert the complete range-call order (including the discarded
third-stock roll) for all three farewell outcomes, and both runtime fixtures
compare the selected variant. The earlier stock-only registration omitted this
draw and therefore was not equivalent to the complete source Create event.

The same audit corrected the sound descriptor: soundPlayOL argument two is
`soundPriority`, passed to audio_play_sound, not a volume percentage. Gain is
read from the playing sound and multiplied by gameVolume. `Sound::priority`
now names the retained field correctly so future playback cannot mistake 90/50
for gain. No sound playback is added by this correction.

## Shared menu presentation clock

Shop Create initializes cIndex=0 and cSp=0.25. Shop Draw's VM adds cSp,
compares cIndex > sprite_get_number(sprShopArrow)-1, and subtracts the full
frame count. Resource 484 has four frames: the thirteenth active draw therefore
produces -0.75, not 3.25 or zero. Shared menu state retains exact signed quarter
frames, without introducing target floating point or guessing how a negative
subimage should resolve to a bitmap. Wobble moves one pixel toward zero before
presentation, including when the menu is closed; cursor phase advances only
while active and is not reset on reentry.

Both runtime adapters advance this once per simulated frame for surviving shop
counters. Rendering/capture calls must remain read-only, so repeated back-buffer
composition cannot speed up the animation. Focused tests compare 10,000 updates
to the recovered arithmetic and exercise both wobble directions and inactive
phase retention. The paired runtime fixture also compares phase and wobble.
Active-menu composition and original runner subimage resolution remain pending;
this clock change is not evidence of on-screen menu parity.

## Shared menu layout contract

`ShopMenuLayout.h` emits the supported Shop Draw branches in source order:
description panel 489, cursor 484 (signed quarter-frame phase), numbered overlay
498, full-size item frame 0, and centered/middle-anchored text requests. The
panel tracks cameraHeight/2-85, while the case cursor remains world-anchored.
Wobble offsets the cursor and the item/text in opposite directions. Sold-out
and Leave replace the item block; an unaffordable price requests red text.
The legacy state-zero choices are retained. State two (selling) explicitly
returns unsupported instead of silently presenting a fabricated interface.

Text requests retain semantic IDs and numeric values, not copyrighted localized
payloads. The original English entries are in read-only data/language.ini;
backends still need their extracted text/raster assets and middle alignment.
`make test-shop-menu-layout` checks command order, IDs, frames, world/viewport
coordinates, camera-height changes, affordability, sold-out/Leave, inactive and
retired counters. This is a layout contract, not connected rendering yet;
offline menu assets, negative-subimage resolution and on-target composition
remain the next integration work.

## Offline menu artwork bank

`make verify-amiga-shop` creates/reproduces generated/amiga-shop.dwap with the
existing pinned Steam-source hash and DWAP v2 format. Its 18 records contain
the six full-size item icons (frame 0), all four cursor frames, their four
offline 270-degree rotations (frame IDs 768..771), description panel 489:0,
and numbered overlays 498:1..3. Every plane/mask pixel is decoded and checked
against the source image during conversion; rotated variants use transformed
origins, matching the existing quarter-turn asset convention.

The pack is 9092 bytes, SHA-256
2688e9d781e61c29a25d182098c6a7b5f08aeee64571df6c56c32b9a1a702117.
It is deliberately not appended to the resident gameplay pack: the latter
remains byte-for-byte unchanged, with its existing Chip-RAM budget. This bank
is not loaded by the executable yet. Localized text extraction/rasterization,
bank lifetime management and backend composition remain required before any
claim of visible shop-menu support. Generated artwork stays ignored by Git.

The bank now also extracts English shop labels and six item names/descriptions
from data/language.ini, rather than committing copied localized payloads.
Text IDs 65410..65413 cover Exit/Buy/Purchased/Back, 65420..65425 names,
65430..65435 descriptions, and 65440..65450 digits/G for dynamic prices.
The BUY literal comes from scrShopAssignText. Star/hash breaks are normalized
to newlines; each line is centered independently. Masks use font 2 and
center/middle origins; the compositor must apply the source four-direction
black outline and foreground colour. No runtime font conversion is needed.

Current bank: 45 records, 14840 bytes, SHA-256
c32dfd9b893ad557470cd962875ca4c8e044a4d2fcc0004785162b36a21aaed5.
The ignored .manifest.json records both input hashes, extraction tool version
and commit, generator version and output hash. Reproducibility checks include
this provenance. tests/shop-pack-test.py verifies original glyph masks,
multiline alignment, anchors and the source font's absence of kerning for
these strings. Existing game/results pack checks remain unchanged. Runtime
loading, price spacing and original-game vertical alignment validation are
still required; the generated bank is not yet presented by either backend.

`ShopMenuText.h` now resolves semantic layout requests to those offline masks.
It emits the four complete black text passes before the white/red foreground,
including for multi-glyph prices. Prices retain the original eight-pixel space
between digits and G and are centered as a whole string, not as just digits.
The source-font test explicitly checks all ten digits, space and G advances.
Layout tests cover exact pass order, affordability colour, zero, full unsigned
price range and all six description IDs. This compositor uses only integer
operations and fixed scratch storage; adapters still need to call it while
the shop bank is loaded. No on-screen shop verification is claimed yet.

## Native overlay bank lifetime

The native executable now embeds the shop bank in ordinary executable data and
loads it into the existing retired-garden middle gap when a shop menu becomes
active. Results select their own bank before composing their overlay. No extra
Chip allocation is added: staging-ring and HUD-art partitions remain unchanged.
The loader validates capacity and planar-only payload size, drains pending
blits before replacing bytes, invalidates the other bank's pointer, and avoids
copies when the requested bank is already resident. Failure returns code 20.

The gameplay-cache verifier explicitly swaps to the shop bank, checks every
byte and repeated selection, restores results, then checks results and HUD art
after terrain staging/side-cache probes. This exercises bank ownership without
inserting a non-original shop into level 1-1. Visible menu drawing is still not
connected; bank integrity does not establish original-game presentation parity.

Validation: bounded SKIP_TO_GAME cache run passed 600 ticks/101 fresh terrain
comparisons with bank checks enabled. Resident art remained 274764 bytes and
startup Chip headroom 19344 bytes; task stack retained its 256-byte margin.
Both native 68000 audits passed. Normal build restored after instrumentation.

## Native menu composition

`AmigaShopMenu` now consumes the shared layout/text requests using the offline
bank on the HUD playfield. It retains bounded command arrays in static/Fast
storage and computes dirty bounds from changed old/new asset commands. Only
that rectangle is cleared and recomposed, including overlapping unchanged
commands; the hardware-scrolled terrain ring is not repainted. Closing clears
the old overlay, and results/death-flash ownership resets retained state.
Text uses opaque-black pen 3 for outlines and white/red pens for foreground.
The negative cursor phase resolves to the counter's zero image index; positive
phases select integer subimages and Leave selects offline rotated variants.

Host tests compare 64 incremental frames against fresh composition across
camera motion, wobble, navigation, sold-out stock and closing. Sentinel world
bytes prove this path never writes the terrain ring. The native cache verifier
also exercises five shop states with the real Chip blitter and compares each
HUD byte against scalar composition before restoring the gameplay HUD.

This connects native drawing for explicitly registered shops, not production
shop generation. SDL menu integration, original-runner screenshot comparisons,
interaction prompts/merchant speech and natural side-room progression remain
open. Matching the scalar compositor alone does not prove original-game pixels
or event/depth ordering, and menu frame-time profiling remains necessary.

## SDL menu adapter

Game::renderMain now submits the same shop layout and five-pass text composition
as the native overlay. Host labels are generated beside the planar bank from
the same English input (amiga-shop.text.h, ignored), and their reproducibility
is checked with the bank/provenance. Build prerequisites generate the header
before compiling Game; no localized payload was added to tracked game code.
The adapter converts center/middle anchors to the SDL font renderer's top
anchor using the shared ten-pixel glyph canvas and twelve-pixel line spacing.
Sprites retain original resource IDs; the host rotates the Leave arrow while
Amiga selects the corresponding offline variant.

The Game renderer fixture explicitly registers a shop, checks landscape panel
and item-name anchors, repeats rendering without advancing cursor state, and
checks that closing stops submissions. This does not yet establish a Windows
pixel match or natural shop appearance. Production generation, merchant speech,
prompts and end-to-end original-game comparisons remain open.

## Generated instance dispatch

Both live builder creation callbacks now route object 277 directly to shared
shop registration. This occurs synchronously inside pattern traversal, so
stock and farewell RNG draws precede subsequent cell/Create draws. SDL does
not enqueue a duplicate generic Shop instance; Amiga does not store the Shop
as a static terrain tile. Shared shop fixture revisions handle its cached art.
Capacity failure propagates through the existing generation-failure path.

This does not enable shops in ordinary 1-1, which scrLevelPat1 suppresses.
The builder still lacks full stage/style configuration: source rightRoomAmt
can be three, style 1 uses choose(0,0,0,0,0,0,1), style 4 forces a shop on the
last side room without that probability draw, and membership forces a shop
after the style-specific choice. Those rules, later-stage setup and a seeded
generated-shop replay are still required. Existing paired fixtures exercise
registration directly; baseline regressions do not cover natural object-277
emission because the current ordinary 1-1 path deliberately emits none.

## Configurable cavern builder setup

AreaOneBuilder now accepts level/style/member/first-boot setup while its old
reset overload preserves ordinary 1-1 defaults. It draws choose(2,2,3) before
the first-level override, applies membership's extra room, retains the draw
before forcing the member's first room to zero, and spaces up to four rooms
against all preceding choices with the source 101-rejection sentinel. Level
length and prelude length follow level/first-boot setup. The source's accepted
101st candidate is no longer incorrectly discarded by the old simplified loop.

Shop choice now distinguishes styles 1 and 4 and applies membership after the
style-specific draw. Nonshop pattern selection avoids every previous side-room
pattern and stops rerolling after the source 61-attempt bound. Seeded tests
traverse complete generated maps: 32 member configurations emit exactly one
Shop object, and 32 nonmember style-four first levels defer that shop until the
last side room. Existing visible-builder/RNG regression checks remain in place.

This is builder configuration support, not full support for those styles or
later stages. Runtime stage/style selection still uses ordinary 1-1 defaults;
enemy scaling, stage transitions and complete source Create effects remain
separate work. The generated-map fixtures collect Shop commands without running
shop stock creation, so they are not yet full-world RNG parity evidence.

## Generated-shop synchronous replay

The native terrain adapter now forwards explicit AreaOneBuilder::Setup through
resetRoom; the old overload keeps the default setup. A 16-seed host-side paired
test runs the core builder and native terrain adapter through complete
style-four first-level generation. Both Create observers register emitted Shop
objects immediately on their builder's random stream, including stock rerolls
and farewell selection, rather than merely counting commands. The test checks
the full RNG state after every generation step, subsequent pattern/frontier
selection, ordered object IDs/positions, shop coordinates, stock, discounted
prices and farewell variant. Each path reaches the end with exactly one shop.

The test isolates synchronous generation: it does not step all enemy/merchant
events between builder steps, use actual runtime stage selection, or prove
Windows equivalence. Gameplay still defaults to ordinary 1-1, and its setup and
shop price context must be wired together when other styles/stages become
available. No test-only forced shop was inserted into production 1-1.

## Interaction sign state

Shop Step's final scrCheckThisSignStep call is now shared. Eligible contact
raises checkThis to at least 2 and adds binary64 0.3 while it is <=5.1.
Otherwise it adds binary64 0.1, then subtracts 2 once if above 2. It runs after
interaction/mask changes, including while speaking; Draw's speaking/criminal
suppression is a separate concern. InteractionPrompt uses HudReal, avoiding
target floating-point helpers and decimal-counter drift.

Tests compare all phase bits and floor frames over 10,000 approach/leave
updates against host binary64 arithmetic; the paired runtime fixture compares
prompt state as well. Native build and 68000 audits pass. Rendering sprite 707
is not connected yet: neither an absent prompt nor matching internal phase
constitutes original-game visual parity.

## Interaction sign rendering

Both drawing adapters now submit original sprite 707 at counter.x,counter.y-20
using the shared prompt frame. The shared draw helper suppresses speaking,
criminal and retired-counter signs. Current runtime callers still supply the
supported noncriminal state; theft/criminal behavior is not implemented by this
change. Amiga's six planar and DMA frames are converted offline, use existing
hardware-channel assignment/BOB fallback, and never enter the static terrain
cache. Actor capacity reserves space for a merchant plus a sign per shop.

Host tests check source resource/position/frame and visibility guards; renderer
tests require all six DMA frames and run 20580 offline sprite pixel comparisons.
The existing 278000-byte resident-art budget is unchanged. Ordinary gameplay
cache runs still contain no shop; that regression validates memory/asset
integration, not a live prompt screenshot against Windows.

The 600-tick target regression initially exposed insufficient task-stack
margin. Moving only the verifier's shop fixture improved but did not resolve
it; moving the long-lived live terrain builder to static Fast storage resolved
the remaining pressure. No stack allowance or safety threshold was increased.
The final bounded run passed all 101 terrain comparisons, shop overlay checks
and the original 256-byte stack guard on a 4096-byte task stack. Resident art
is now 276684 bytes and startup Chip headroom 17424 bytes. Normal native build
restored after the verifier run; SDL build and renderer tests also pass.
