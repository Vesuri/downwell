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
These deferred effects and live world/menu integration are the next work, not
implemented by this inventory module.

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
