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
