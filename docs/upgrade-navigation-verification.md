# Repeated upgrade selection verification

The native clear fixture previously navigated right only once, using cumulative
clear-frame 80. Confirmation began at cumulative frame 100, so the second
entry did not independently navigate its menu. Its pixel checks remained
useful, but did not prove left/right input and retained artwork on both entries.

The revised `DOWNWELL_CLEAR_VERIFY` tape counts visible-choice frames separately
for each entry. It holds right on 12–13, left on 20–21 and 28–29, then right on
36–37, before confirming from 52 onward. Production input edge filtering is
used unchanged. Every visible frame checks the expected cursor, so a held
left key repeating on its second frame fails the test. Each room must visit
cursors 0, 1 and 2. This does not cover the optional fourth offer or all styles.

The existing fresh planar composition comparisons still run throughout
entrance, navigation and departure; the gate also requires two re-entries,
60 third-level gameplay updates, exit-field comparisons and the stack guard.
All new state/input/assertions compile out of ordinary builds. No game rules,
RNG draws or scene timing are changed.

The baseline recheck reached 300 comparison frames without a mismatch but
hit its 180-second deadline (`/tmp/downwell-post-cache-clear.log`). This is an
incomplete run, not a pass. The revised run uses a 360-second hard deadline
to allow the additional comparison-heavy navigation frames. That timeout is
not a gameplay performance measurement.

Host selection, controller, motion, presentation and session checks pass
(`/tmp/downwell-upgrade-navigation-host.log`). They include binary64 comparison
of 10,000 motion and 20,000 carousel updates. Exits remain injected in this
native fixture; natural full-level traversal and Windows rasterization remain
separate verification requirements.

The 360-second-bounded native run passes: 466 planar upgrade-screen frames,
104 visible-choice frames (52 per entry), both cursor masks equal to 7,
192 retained exit-field comparisons, two re-entries and 60 final gameplay
updates. Pixel/cursor failures are zero and the stack guard is intact.
Evidence: `/tmp/downwell-upgrade-navigation-native.log` and
`/tmp/downwell-upgrade-navigation-native-gdb.log`. The normal startup build
is restored with clean 68000 instruction and helper audits.
