// DOWNWELL_CLEAR_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 3
volatile ULONG gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures;
volatile ULONG gDownwellClearChoiceInputFrames, gDownwellClearCursorMasks[2];
volatile ULONG gDownwellClearReentries, gDownwellClearGameplayUpdates;
volatile ULONG gDownwellClearFieldFrames;
volatile ULONG gDownwellClearTick;
volatile ULONG gDownwellClearPhase;
volatile ULONG gDownwellClearOracleBytes;
const UBYTE* gDownwellClearActualHud;
const UBYTE* gDownwellClearExpectedHud;
const UBYTE* gDownwellClearActualGame;
const UBYTE* gDownwellClearExpectedGame;
__attribute__((noinline, used)) void downwellClearProbe() { __asm__ volatile("" ::: "memory"); }
__attribute__((noinline, used)) void downwellClearReady() { __asm__ volatile("" ::: "memory"); }
#endif
#if DOWNWELL_VERIFY_PART == 25
                UBYTE* expectedGame[2]={clearOracle.game(0),clearOracle.game(1)};
                UBYTE* expectedHud[2]={clearOracle.hud(0),clearOracle.hud(1)};
                static AmigaClearPresenter oracle;
                if (entering) oracle.reset();
                if (bankChanged) oracle.artworkChanged();
                AmigaRenderer reference;
                amigaRendererInit(&reference, expectedGame[back], expectedHud[back]);
                AmigaClearPresenter::Frame expected;
                UBYTE* actualGame=clearOracle.game(2);
                UBYTE* actualHud=clearOracle.hud(2);
                gDownwellClearActualHud = actualHud; gDownwellClearExpectedHud = expectedHud[back];
                gDownwellClearActualGame = actualGame; gDownwellClearExpectedGame = expectedGame[back];
                // Production banks omit label frame0 when only its offline
                // outline is drawn. The scalar oracle reads original frames
                // from the immutable full source pack in ordinary memory.
                if (!oracle.render(back, reference, upgradeSource.view().data,
                        upgradeSource.view().data + upgradeSource.view().bytes, room, snapshot, port.upgrades(), expected))
                    ++gDownwellClearFailures;
                // Compare visible pixels, not storage: the new canvases are
                // world-space narrow planes, the oracle uses a wide ring.
                for (unsigned layer = 0; layer < 2; ++layer) {
                    const UBYTE* compact = layer ? frame.layer : frame.border;
                    const UBYTE* wide = layer ? expectedHud[back] : expectedGame[back];
                    UBYTE* actual = layer ? actualHud : actualGame;
                    const unsigned size = layer ? downwellLayerBytes : downwellGameRingBytes;
                    for (unsigned i = 0; i < size; ++i) actual[i] = wide[i];
                    for (unsigned plane = 0; plane < 2; ++plane)
                        for (unsigned y = 0; y < downwellDisplayHeight; ++y) {
                            const unsigned row = layer ? y : (expected.origin + y) % downwellGameRingHeight;
                            for (unsigned word = 0; word < 10; ++word) {
                                const unsigned index = plane * (size / 2) + row * 48 + 13 + word * 2;
                                const UBYTE* src = compact + plane * AmigaSelectionLayer::planeBytes + y * 22 + word * 2;
                                const unsigned value = (unsigned(src[0]) << 8) | src[1];
                                const unsigned expectedWord = ((unsigned(wide[index]) << 14) |
                                    (unsigned(wide[index+1]) << 6) | (wide[index+2] >> 2)) & 0xffff;
                                if (value != expectedWord) ++gDownwellClearFailures;
                                actual[index] = (actual[index] & 0xfc) | (value >> 14);
                                actual[index+1] = value >> 6;
                                actual[index+2] = (actual[index+2] & 3) | (value << 2);
                            }
                        }
                }
                ++gDownwellClearFrames;
                if (snapshot.choices) ++gDownwellClearChoices;
                if (snapshot.choices && room.selection.visible && gDownwellClearReentries < 2) {
                    ++gDownwellClearChoiceInputFrames;
                    gDownwellClearCursorMasks[gDownwellClearReentries] |= 1u << room.selection.cursor;
                    const unsigned frame = gDownwellClearChoiceInputFrames;
                    const unsigned expectedCursor = frame >= 13 && frame <= 20 ? 2 :
                        frame >= 29 && frame <= 36 ? 0 : 1;
#ifndef DOWNWELL_NATURAL_VERIFY
                    if (room.selection.cursor != expectedCursor) ++gDownwellClearFailures;
#else
                    (void)expectedCursor;
#endif
                }
                gDownwellClearPhase = 4;
                if (gDownwellClearFrames == 1 || gDownwellClearFrames % 60 == 0)
                    downwellClearProbe();
#endif
#if DOWNWELL_VERIFY_PART == 28
            gDownwellClearTick = port.tick();
            if (port.tick() % 60 == 0) downwellClearProbe();
            if (gDownwellClearReentries) {
                ++gDownwellClearGameplayUpdates;
#ifdef DOWNWELL_NATURAL_VERIFY
                naturalRoute.check(port);
                gDownwellNaturalFailures = naturalRoute.failures;
                gDownwellNaturalReturnFrames = naturalRoute.returnFrames;
#else
                if (gDownwellClearReentries == 2 && gDownwellClearGameplayUpdates == 60) downwellClearReady();
#endif
            }
#endif
