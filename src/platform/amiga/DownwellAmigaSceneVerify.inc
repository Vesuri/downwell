// DOWNWELL_SCENE_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 4
extern "C" { volatile ULONG gDownwellSceneStackFailureStage; }
extern "C" __attribute__((noinline,used)) void downwellSceneStackFailed(){
    __asm__ volatile("" ::: "memory");
}
__attribute__((noinline)) static void checkSceneStack(ULONG stage){
    if(gDownwellSceneStackFailureStage||!gDownwellStackGuardReady)return;
    const volatile ULONG* guard=(const volatile ULONG*)SysBase->ThisTask->tc_SPLower;
    for(unsigned i=0;i<64;++i)if(guard[i]!=0x57ac6a4d){
        gDownwellSceneStackFailureStage=stage;downwellSceneStackFailed();return;
    }
}
#endif
#if DOWNWELL_VERIFY_PART == 6
volatile LONG gDownwellTerrainFirstMismatch[8];
volatile ULONG gDownwellColdStageRows;
volatile ULONG gDownwellSplashCheckedFrames[7];
volatile ULONG gDownwellSplashPublicationFailures;
volatile const UWORD* gDownwellMenuSpriteOracle;
volatile ULONG gDownwellMenuCopperPatchFailures;
volatile ULONG gDownwellConsumableChecks, gDownwellConsumableFailures;
#ifdef DOWNWELL_CAMERA_VERIFY
volatile ULONG gDownwellCameraColumns, gDownwellCameraTransitions;
volatile ULONG gDownwellCameraStaged, gDownwellCameraFallback;
volatile ULONG gDownwellCameraFrameChecks;
volatile ULONG gDownwellCameraLocks, gDownwellCameraReleases;
#endif
volatile ULONG gDownwellCaptureTick;
#ifdef DOWNWELL_GEMHIGH_VERIFY
volatile ULONG gDownwellGemHighCapture = 1;
#else
volatile ULONG gDownwellGemHighCapture;
#endif
volatile ULONG gDownwellGemHighVisibleShots;
volatile ULONG gDownwellGemHighStreak;
volatile LONG gDownwellCapturePlayerXQ8, gDownwellCapturePlayerYQ8;
volatile LONG gDownwellCaptureViewX, gDownwellCaptureViewY;
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
volatile ULONG gDownwellResultsRestartCount;
volatile ULONG gDownwellResultsRestartMask;
// Only the existing results-verification build admits this fault injection.
// A real checked callback rejects Draw. The emulator debugger drops memory
// writes, so the target reads a disposable fixture marker before takeover.
volatile ULONG gDownwellEncounterRejectDraw;
volatile ULONG gDownwellEncounterDrawRejected;
volatile ULONG gDownwellEncounterFailurePublications;
volatile ULONG gDownwellResultsHudChecks, gDownwellResultsHudFailures;
volatile ULONG gDownwellResultsBankChecks, gDownwellResultsBankFailures;
__attribute__((noinline, used)) void downwellResultsRestartReady() { __asm__ volatile("" ::: "memory"); }
#endif
#endif
#if DOWNWELL_VERIFY_PART == 7
static downwell::Input sceneVerificationInput(
    const AmigaSceneRuntime& scenes, bool styleFrameReported,
    bool lockedStyleFrameReported,
    bool gameplayLanded, const AmigaPortRuntime& port) {
    downwell::Input input;
    if (scenes.scene() == AmigaScene::menu)
        input = downwell::gardenRouteInput(scenes.menuPlayerX());
    else if (scenes.scene() == AmigaScene::styleSelect &&
             scenes.styleYOffset() == 0 && styleFrameReported &&
             !scenes.styleSelected()) {
        if (!lockedStyleFrameReported)
            input.right = scenes.styleCursor() == 0;
        else if (scenes.styleCursor() != 0)
            input.left = true;
        else
            input.fire = true;
    }
    else if (scenes.scene() == AmigaScene::gameplay && gameplayLanded) {
#ifdef DOWNWELL_SCENE_DESCENT_VERIFY
        input = downwell::sceneDescentVerificationInput(port.tick(),
            240L * 256 + port.player().offsetXFixed());
#else
        const unsigned movementPhase = (port.tick() / 120u) & 1u;
        input.right = movementPhase == 0;
        input.left = movementPhase != 0;
        const unsigned firePhase = port.tick() % 16u;
        input.fire = firePhase == 0 || firePhase == 3;
#endif
    }
    return input;
}
#endif
#if DOWNWELL_VERIFY_PART == 9
            // Compare the cached row transfer against fresh composition at
            // the same ring position, including the physical wrap boundary.
            AmigaVerificationScratch scratch;
            auto expected=reinterpret_cast<UWORD (*)[2][terrainRasterWords]>(
                scratch.data<downwellDisplayHeight*2*terrainRasterWords*sizeof(UWORD)>());
            for (int y = 0; y < clipHeight; ++y)
                for (unsigned plane = 0; plane < 2; ++plane) {
                    const unsigned physical = (renderer->gameOrigin + clipY + y) % downwellGameRingHeight;
                    const UWORD* chip = (const UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                        physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                    for (unsigned word = 0; word < terrainRasterWords; ++word)
                        expected[y][plane][word] = chip[word];
                }
            terrainRaster.ready = false;
            amigaRendererClearGameRows(renderer, clipY, clipHeight, 0);
            drawMainWorldSpan(renderer, rooms, room, builders, fragments, fragmentCount,
                decorations, decorationCount, runtime, viewX, viewY, clipY, clipHeight, clipX, clipWidth);
            AmigaHardware::blitterDrain();
            terrainRaster.ready = true;
            for (int y = 0; y < clipHeight; ++y)
                for (unsigned plane = 0; plane < 2; ++plane) {
                    const unsigned physical = (renderer->gameOrigin + clipY + y) % downwellGameRingHeight;
                    const UWORD* chip = (const UWORD*)(renderer->game + plane * downwellGameRingPlaneBytes +
                        physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                    for (unsigned word = 0; word < terrainRasterWords; ++word)
                        if (expected[y][plane][word] != chip[word]) {
                            if (!gDownwellTerrainRasterFailures) {
                                const LONG details[] = {viewX, firstWorldY, clipHeight, y,
                                    LONG(plane), LONG(word), expected[y][plane][word], chip[word]};
                                for (unsigned i=0;i<8;++i) gDownwellTerrainFirstMismatch[i]=details[i];
                            }
                            ++gDownwellTerrainRasterFailures;
                        }
                }
            ++gDownwellTerrainRasterChecks;
#endif
#if DOWNWELL_VERIFY_PART == 11
extern "C" { volatile ULONG gDownwellOverlayCopperChecks, gDownwellOverlayCopperFailures; }
static bool verifyOverlayCopper(const Bitmap& game, const Bitmap& hud,
                                const Sprite& nullSprite, const Palette& palette) {
    // Isolated, never-published list: exercise real framework word patches on
    // the 68000 without redirecting active DMA or repainting a display buffer.
    static UWORD words[downwellCopperWords], before[downwellCopperWords];
    DownwellGameplayCopperList list(words, downwellCopperWords);
    Bitmap alternate(game.data, downwellVisibleWidth, downwellDisplayHeight, 2, false, false, downwellDmaWidth);
    if (!list.build(game, hud, nullSprite, nullptr, 0, nullSprite, palette, 0)) return false;
    const unsigned offset = list.moveOffset(bpl2pth);
    const unsigned length = list.size();
    for (unsigned origin = 0; origin < downwellGameRingHeight; ++origin) {
        if (!list.update(game, origin)) return false;
        for (unsigned buffer = 0; buffer < 2; ++buffer) {
            for (unsigned i = 0; i < length; ++i) before[i] = words[i];
            const Bitmap& target = buffer ? hud : alternate;
            if (!list.updateOverlay(target) || list.size() != length) return false;
            for (unsigned plane = 0; plane < 2; ++plane) {
                const unsigned at = offset + plane * 4;
                const ULONG wanted = (ULONG)target.data + downwellFetchCropBytes + plane * downwellPlaneBytes;
                if (words[at] != bpl2pth + plane * 8 || words[at + 2] != words[at] + 2 ||
                    (((ULONG)words[at + 1] << 16) | words[at + 3]) != wanted) return false;
            }
            for (unsigned i = 0; i < length; ++i) {
                if (i == offset + 1 || i == offset + 3 || i == offset + 5 || i == offset + 7) continue;
                if (words[i] != before[i]) return false;
            }
            ++gDownwellOverlayCopperChecks;
        }
    }
    for (unsigned i = 0; i < length; ++i) before[i] = words[i];
    Bitmap invalid(game.data, downwellVisibleWidth, downwellGameRingHeight, 2, false, false, downwellDmaWidth);
    if (list.updateOverlay(invalid) || list.update(game, downwellGameRingHeight)) return false;
    for (unsigned i = 0; i < length; ++i) if (words[i] != before[i]) return false;
    return true;
}

static void exposeGameplayFrame(const AmigaMemory& memory, const Bitmap& display, unsigned buffer,
                                const AmigaPortRuntime& port) {
    gDownwellCaptureTick = port.tick();
    gDownwellCapturePlayerXQ8 = 240L * 256 + port.player().offsetXFixed();
    gDownwellCapturePlayerYQ8 = 176L * 256 + port.player().offsetYFixed();
    gDownwellCaptureViewX = 160 + port.cameraOffsetX();
    gDownwellCaptureViewY = 34 + port.cameraOffsetY();
    gDownwellGameplayGame = (const UBYTE*)display.data;
    AmigaVerificationScratch scratch;
    UBYTE* expected=scratch.data<downwellGameRingBytes>();
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte) expected[byte] = memory.game()[byte];
    static downwell::TimeFieldOutlines fieldOracle;
    fieldOracle.set(port.timeFields(), port.goalField(), port.levelExitPresent(),
                    port.goalFieldBirthBoundary(), 160 + port.cameraOffsetX(),
                    34 + port.cameraOffsetY(), downwellPlayfieldX,port.startTimeField());
    if (fieldOracle.visible) {
        AmigaRenderer oracle;
        amigaRendererInit(&oracle, expected, nullptr);
        amigaRendererSetGameOrigin(&oracle, port.ringOrigin());
        amigaRendererSetClipRect(&oracle, downwellPlayfieldX, 0, downwellPlayfieldWidth, downwellDisplayHeight);
        fieldOracle.draw([&](int x, int y, int pen) { amigaRendererFillRect(&oracle, x, y, 1, 1, pen); });
    }
    if (const auto* line = gameplayRays[buffer].line()) {
        AmigaRenderer oracle;
        amigaRendererInit(&oracle, expected, nullptr);
        amigaRendererSetGameOrigin(&oracle, gameplayRays[buffer].origin());
        amigaRendererSetClipRect(&oracle, downwellPlayfieldX, 0, downwellPlayfieldWidth, downwellDisplayHeight);
        line->draw([&](int x, int y) { amigaRendererFillRect(&oracle, x, y, 1, 1, 2); });
    }
    // Test damage once per row, not two software divisions per plane byte.
    // Dirty rows include deliberately drawn BOBs, unlike pristine backing.
    for (unsigned row = 0; row < downwellGameRingHeight; ++row) {
        if (gameplayRows.dirty(row, buffer)) continue;
        for (unsigned plane = 0; plane < 2; ++plane) {
            const unsigned offset = plane * downwellGameRingPlaneBytes + row * downwellBytesPerPlaneLine;
            gDownwellGameBufferFailures += amigaBufferDifferences(
                static_cast<const UBYTE*>(display.data) + offset, expected + offset, downwellBytesPerPlaneLine);
        }
    }
    gDownwellGameplayHud = memory.hud();
    gDownwellGameplayPlayerSprite = memory.playerSprite(buffer);
    gDownwellGameplayActorSprites = memory.bulletSprite() +
        buffer * gameplayRegularSpriteCount *
            (bulletSpriteBytes / sizeof(UWORD));
    gDownwellGameplayOverflowSprite = memory.enemySprite(buffer);
    gDownwellGameplayRingOrigin = (UWORD)port.ringOrigin();
}
#endif
#if DOWNWELL_VERIFY_PART == 12
    // The emulator debugger cannot reliably write target memory. Initialize
    // the canary on the CPU, below this frame with room for this loop itself.
    ULONG currentStack;
    __asm__ volatile("move.l %%sp,%0" : "=r"(currentStack));
    volatile ULONG* stackGuard = (volatile ULONG*)SysBase->ThisTask->tc_SPLower;
    if (currentStack > (ULONG)stackGuard + 512) {
        for (unsigned word = 0; word < 64; ++word)
            stackGuard[word] = 0x57ac6a4d;
        gDownwellStackGuardReady = 1;
    }
#endif
#if DOWNWELL_VERIFY_PART == 14
    // Exercise in-place recreation on the real 68000 before binding terrain.
    // Finish at the ordinary entry state so the complete scene oracle applies.
    if (port.restartFromResults(downwell::DeathMenuSequence::Action::StyleSelect, 42,
            initialRingOrigin) != downwell::ResultsDestination::styleSelect ||
        port.runStage().area != 0 ||
        port.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 2,
            initialRingOrigin) != downwell::ResultsDestination::gameplay ||
        port.health() != 4 || port.runStage().area != 1 || port.tick() != 0)
        return 20;
    checkSceneStack(4);
#endif
#if DOWNWELL_VERIFY_PART == 16
    const UWORD* expectedSplashCopper = 0;
    bool sceneGameplayLandedReported = false;
    bool sceneGameplayActiveReported = false;
    bool sceneMenuTitleReported = false;
    bool sceneMenuEntryReported = false;
    bool sceneMenuWakeFrameReported = false;
    bool sceneStyleFrameReported = false;
    bool sceneLockedStyleFrameReported = false;
    bool sceneSelectedStyleFrameReported = false;
    bool sceneSplashFrameReported = false;
    bool sceneControlsFrameReported = false;
    bool sceneCreditsFrameReported = false;
    int sceneGameplayLandedCameraY = 0;
#endif
#if DOWNWELL_VERIFY_PART == 18
            downwell::Input liveInput = sceneVerificationInput(
                scenes, sceneStyleFrameReported,
                sceneLockedStyleFrameReported,
                sceneGameplayLandedReported, port);
#ifdef DOWNWELL_SHOP_VERIFY
            if (scenes.scene() == AmigaScene::gameplay)
                liveInput = shopVerification.input(port);
#endif
#ifdef DOWNWELL_CAMERA_VERIFY
            // Synthetic flat collision fixture, not a natural shop traversal.
            // Real movement/camera and all display staging/publication still run.
            if (scenes.scene() == AmigaScene::gameplay) {
                liveInput = {};
                const unsigned phase = port.tick() % 320;
                liveInput.right = phase < 80 || phase >= 240;
                liveInput.left = !liveInput.right;
            }
#endif
#ifdef DOWNWELL_LASER_VERIFY
            if (sceneGameplayLandedReported && !port.upgrades().levels[12]) port.grantUpgrade(12);
#endif
            if (gDownwellGemHighCapture == 1 && sceneGameplayLandedReported) {
                // Synthetic accumulated combo only; normal landing rewards,
                // controller selection, firing and rendering do the rest.
                const_cast<downwell::RunCombo&>(port.runCombo()).state.count = 8;
                liveInput = {}; // Let the grounded reward finish before jumping.
                gDownwellGemHighCapture = 2;
            }
#ifdef DOWNWELL_CLEAR_VERIFY
            if (port.levelClearActive()) {
                liveInput = {};
                const unsigned frame = gDownwellClearChoiceInputFrames;
                // Each room independently exercises all three choices and
                // held-key edge filtering before confirmation is allowed.
                liveInput.right = frame == 12 || frame == 13 || frame == 36 || frame == 37;
                liveInput.left = frame == 20 || frame == 21 || frame == 28 || frame == 29;
                liveInput.fire = frame >= 52;
            }
#endif
#ifdef DOWNWELL_NATURAL_VERIFY
            if (scenes.scene() == AmigaScene::gameplay)
                liveInput = naturalRoute.input(port, terrainCollision, gDownwellClearGameplayUpdates);
#endif
#ifdef DOWNWELL_RESULTS_RESTART_VERIFY
            // Inject only results readiness. Confirmation, reset, room changes,
            // resource reuse and subsequent gameplay take the production path.
            if (scenes.scene() == AmigaScene::gameplay && port.tick() == 12 &&
                gDownwellResultsRestartCount < 3) {
                auto& results = const_cast<downwell::DeathResults&>(port.deathResults());
                results = {};
                results.open(true, 0, 0);
                results.menu.sequenceOver = true;
                results.menu.visible[5] = true;
                results.menu.cursor = gDownwellResultsRestartCount;
                liveInput = {};
            } else if (scenes.scene() == AmigaScene::gameplay && port.tick() == 13 &&
                       gDownwellResultsRestartCount < 3) {
                // Present the overlay for a full frame before confirmation.
                liveInput = {};
                liveInput.fire = true;
            }
#endif
#endif
#if DOWNWELL_VERIFY_PART == 19
                        if (!sceneCreditsFrameReported &&
                            presentedSplash == 3) {
                            sceneCreditsFrameReported = true;
                            gDownwellCreditsFrame = creditsScene.pixels();
                            gDownwellCreditsCopper = platform.publishedCopper();
                            downwellCreditsFrameReady();
                        }
                        if (!sceneSplashFrameReported &&
                            presentedSplash == 2) {
                            sceneSplashFrameReported = true;
                            gDownwellSplashFrame = publisherScene.pixels();
                            downwellSplashFrameReady();
                        }
                        if (!sceneControlsFrameReported &&
                            presentedSplash == 4) {
                            gDownwellControlsCopper = platform.publishedCopper();
                            sceneControlsFrameReported = true;
                            gDownwellControlsFrame = controlsScene.pixels();
                            downwellControlsFrameReady();
                        }
#endif
#if DOWNWELL_VERIFY_PART == 20
                    // Test the patched program against a freshly emitted oracle.
                    // Verification scratch is not nested/reentrant. Keep it
                    // off the 4 KiB AmigaDOS stack shared with game stepping.
                    static UWORD oracleWords[downwellCopperWords];
                    DownwellMenuCopperList oracle(oracleWords, downwellCopperWords);
                    Sprite oraclePlayer(menuPlayerData, playerSpriteHeight);
                    const bool oracleReady = oracle.buildScroll(
                                             menuEntry ? memory.menuEntry() : memory.menuForeground(),
                                             memory.menuBackground(), oraclePlayer,
                                             nullSprite, palette, menuScroll, menuEntry);
                    if (!oracleReady || oracle.size() != gardenScene.copper(back).size())
                        ++gDownwellMenuCopperPatchFailures;
                    else for (unsigned word = 0; word < oracle.size(); ++word)
                        if (oracleWords[word] != gardenScene.copper(back).data()[word])
                            ++gDownwellMenuCopperPatchFailures;
#endif
#if DOWNWELL_VERIFY_PART == 21
                    if (!sceneMenuWakeFrameReported && !menuEntry &&
                        !scenes.menuPlayerNapping() &&
                        scenes.menuPlayerVisible() &&
                        scenes.menuPlayerSprite() == 27) {
                        sceneMenuWakeFrameReported = true;
                        // Test-only oracle; never used for presentation. The
                        // generic builder uses the gameplay beam origin, while
                        // garden DMA retains its independent fetch geometry.
                        amigaRendererBuildHardwareSprite(
                            frontPack, frontPackEnd,
                            (UWORD)scenes.menuPlayerSprite(),
                            (UWORD)scenes.menuPlayerFrame(),
                            downwellPlayfieldX + scenes.menuPlayerX() -
                                (scenes.menuCameraX() - 80) +
                                downwellMenuSpriteHStart - downwellDisplayHStart,
                            scenes.menuPlayerY() - (scenes.menuCameraY() - 142),
                            scenes.menuPlayerFacing() < 0,
                            memory.playerSprite(back), playerSpriteHeight);
                        gDownwellMenuSpriteOracle = memory.playerSprite(back);
                        gDownwellMenuFrameCapture.foregroundOffset =
                            menuScroll.foregroundOffset;
                        gDownwellMenuFrameCapture.backgroundOffset =
                            menuScroll.backgroundOffset;
                        gDownwellMenuFrameCapture.scrollControl =
                            menuScroll.scrollControl;
                        gDownwellMenuFrameCapture.titlePartCount = 0;
                        gDownwellMenuForeground = memory.menuForeground();
                        gDownwellMenuBackground = memory.menuBackground();
                        gDownwellMenuPlayerSprite =
                            menuPlayerData;
                        downwellMenuWakeFrameReady();
                    }
                    if (!sceneMenuTitleReported &&
                        scenes.menuTitleFrame() == 9) {
                        sceneMenuTitleReported = true;
                        gDownwellMenuFrameCapture.foregroundOffset =
                            menuScroll.foregroundOffset;
                        gDownwellMenuFrameCapture.backgroundOffset =
                            menuScroll.backgroundOffset;
                        gDownwellMenuFrameCapture.scrollControl =
                            menuScroll.scrollControl;
                        gDownwellMenuFrameCapture.titlePartCount = 0;
                        gDownwellMenuForeground = memory.menuForeground();
                        gDownwellMenuBackground = memory.menuBackground();
                        // The DMA null sprite is only four words. Fixed-size
                        // captures must not read 64 more words from its next
                        // allocation when the garden player is hidden.
                        static const UWORD hiddenPlayerCapture[68] = {};
                        gDownwellMenuPlayerSprite = menuPlayerData == nullSprite.data()
                            ? hiddenPlayerCapture : menuPlayerData;
                        downwellMenuTitleFrameReady();
                        /* Retain the old verification breakpoint name. */
                        downwellMenuTitleSpriteReady();
                    }
                    if (!sceneMenuEntryReported && menuEntry) {
                        sceneMenuEntryReported = true;
                        gDownwellMenuEntryFrame = memory.menuEntry();
                        gDownwellMenuFrameCapture.foregroundOffset = menuScroll.foregroundOffset;
                        gDownwellMenuFrameCapture.backgroundOffset = menuScroll.backgroundOffset;
                        gDownwellMenuFrameCapture.scrollControl = menuScroll.scrollControl;
                        gDownwellMenuFrameCapture.titlePartCount = 0;
                        gDownwellMenuBackground = memory.menuBackground();
                        gDownwellMenuEntryTitlePartCount = 0;
                        downwellMenuEntryFrameReady();
                        downwellMenuEntryScrollReady();
                    }
#endif
#if DOWNWELL_VERIFY_PART == 22
                    const UBYTE* styleFrame = style.content;
                    AmigaStyleFrameState referenceState;
                    if (!amigaRenderStyleFrame(frontPack, frontPackEnd,
                            memory.sceneCache() + amigaStyleFrameBytes * 2,
                            scenes, memory.sceneCache(), referenceState, true)) break;
                    ++gDownwellStyleReferenceFrames;
                    UBYTE* styleComposite = memory.sceneCache() + amigaStyleFrameBytes;
                    const UBYTE* referenceBorder = memory.sceneCache() + amigaStyleFrameBytes * 2 - scenes.styleTileY() * amigaStyleBytesPerRow;
                    for (unsigned byte = 0; byte < amigaStylePlaneBytes; ++byte) {
                        const UBYTE occupied = styleFrame[byte] | styleFrame[byte + AmigaSelectionLayer::planeBytes];
                        styleComposite[byte] = styleFrame[byte] | (referenceBorder[byte] & ~occupied);
                        styleComposite[byte + amigaStylePlaneBytes] = styleFrame[byte + AmigaSelectionLayer::planeBytes] |
                            (referenceBorder[byte + amigaStyleBorderPlaneBytes] & ~occupied);
                    }
                    // Verification-only hardware compositor: read the actual
                    // DMA headers/art, lower channel first in front, below PF1
                    // text. The separate reveal mask is checked by captures.
                    if(style.hardware)for(int channel=7;channel>=0;--channel){
                        const auto* p=reinterpret_cast<const UBYTE*>(style.sprites[channel]);
                        if(!p)continue;
                        const int startY=p[0]+((p[3]&4)<<6),stopY=p[2]+((p[3]&2)<<7);
                        const int left=p[1]*2+(p[3]&1)-amigaMenuDisplayHStart;
                        for(int y=0;y<stopY-startY;++y)for(int x=0;x<16;++x){
                            const unsigned lo=unsigned(p[4+y*4])*256+p[5+y*4];
                            const unsigned hi=unsigned(p[6+y*4])*256+p[7+y*4];
                            const unsigned pen=unsigned(bool(lo&(32768u>>x)))+2*unsigned(bool(hi&(32768u>>x)));
                            const int dx=left+x,dy=startY-downwellDisplayVStart+y;
                            if(!pen||dx<0||dx>=160||dy<0||dy>=284)continue;
                            const unsigned at=dy*amigaStyleBytesPerRow+dx/8,bit=128u>>(dx&7);
                            if((styleFrame[at]|styleFrame[at+AmigaSelectionLayer::planeBytes])&bit)continue;
                            for(unsigned plane=0;plane<2;++plane){
                                auto& b=styleComposite[at+plane*amigaStylePlaneBytes];
                                b=UBYTE((b&~bit)|((pen&(1u<<plane))?bit:0));
                            }
                        }
                    }
                    for (unsigned byte = 0; byte < amigaStyleFrameBytes; ++byte) {
                        if (styleComposite[byte] != memory.sceneCache()[byte]) {
                            ++gDownwellStyleReferenceFailures;
                            break;
                        }
                    }
#endif
#if DOWNWELL_VERIFY_PART == 23
                    {
                        const auto& copper=styleScene.copper(back);
                        const auto* words=copper.data();
                        const unsigned priority=copper.moveOffset(bplcon2);
                        if(words[priority+1]!=(style.hardware?0x0020:0x0024))
                            ++gDownwellStyleReferenceFailures;
                        for(unsigned channel=0;channel<8;++channel){
                            const unsigned reg=spr1pth+channel*4,at=copper.moveOffset(reg);
                            const auto* expected=style.hardware&&style.sprites[channel]?
                                style.sprites[channel]:nullSprite.data();
                            if(words[at]!=reg||words[at+2]!=reg+2||
                                ((ULONG(words[at+1])<<16)|words[at+3])!=ULONG(expected))
                                ++gDownwellStyleReferenceFailures;
                            if(style.hardware&&style.sprites[channel])for(unsigned other=0;other<channel;++other)
                                if(style.sprites[channel]==style.sprites[other])++gDownwellStyleReferenceFailures;
                        }
                    }
#endif
#if DOWNWELL_VERIFY_PART == 24
                    gDownwellStyleMask = style.mask;
                    gDownwellStyleCopper = styleScene.copper(back).data();
                    if (scenes.styleDitherY() == 528 && !scenes.styleSelected()) {
                        gDownwellStyleFrame = styleComposite;
                        downwellStyleRevealFrameReady();
                    }
                    if (!sceneStyleFrameReported &&
                        scenes.styleYOffset() == 0 &&
                        !scenes.styleSelected()) {
                        sceneStyleFrameReported = true;
                        gDownwellStyleFrame =
                            styleComposite;
                        downwellStyleFrameReady();
                    }
                    if (!sceneLockedStyleFrameReported &&
                        sceneStyleFrameReported &&
                        scenes.styleYOffset() == 0 &&
                        scenes.styleCursor() == 1 &&
                        !scenes.styleSelected()) {
                        sceneLockedStyleFrameReported = true;
                        gDownwellLockedStyleFrame =
                            styleComposite;
                        downwellLockedStyleFrameReady();
                    }
                    if (!sceneSelectedStyleFrameReported &&
                        scenes.styleSelected() &&
                        scenes.styleVerticalDitherY() == 0) {
                        sceneSelectedStyleFrameReported = true;
                        gDownwellSelectedStyleFrame =
                            styleComposite;
                        downwellSelectedStyleFrameReady();
                    }
#endif
#if DOWNWELL_VERIFY_PART == 30
                    AmigaHardware::blitterDrain();
                    const unsigned column = (worldViewX - mainViewX + 160) / 160;
                    for (unsigned y = 0; y < downwellDisplayHeight; ++y) {
                        const unsigned physical = (port.ringOrigin() + y) % downwellGameRingHeight;
                        UWORD scratch[22];
                        const UWORD* cached=terrainRaster.readRow(column,worldViewY+y,scratch);
                        for (unsigned plane = 0; plane < 2; ++plane) {
                            const UWORD* displayed = (const UWORD*)((const UBYTE*)gameDisplay[back].data +
                                plane * downwellGameRingPlaneBytes + physical * downwellBytesPerPlaneLine + terrainRasterX / 8);
                            for (unsigned word = 0; word < terrainRasterWords; ++word)
                                if (displayed[word] != cached[plane * terrainRasterWords + word])
                                    ++gDownwellTerrainRasterFailures;
                        }
                    }
                    amigaRendererSetGameOrigin(&renderer, port.ringOrigin());
                    drawMainWorldSpan(&renderer, rooms, mainRoom, builders,
                        fragments, liveBuilder.fragmentCount(), generatedDecorations,
                        liveBuilder.decorationCount(), &port, worldViewX, worldViewY, 0, downwellDisplayHeight);
#endif
#if DOWNWELL_VERIFY_PART == 35
#ifdef DOWNWELL_CAMERA_VERIFY
            static unsigned cameraFramesToCheck = 0;
            if (cameraXChanged) cameraFramesToCheck = 2;
            if (cameraFramesToCheck) {
                // Check the switched frame and the other alternating display.
                AmigaHardware::blitterDrain();
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                ++gDownwellCameraFrameChecks;
                --cameraFramesToCheck;
            }
#endif
            if (port.tick() == 600) {
                // Deterministic coverage independent of enemy encounters and
                // travel distance: validate every strip of all three views.
                // Use retired staging memory, never the displayed ring.
                AmigaHardware::blitterDrain();
                AmigaRenderer probe = renderer;
                probe.game = stagedBuffer;
                probe.gameOrigin = 0;
                for (int x = mainViewX - 160; x <= mainViewX + 160; x += 160)
                    for (int strip = 0; strip < downwellDisplayHeight; strip += 16) {
                        const int count = strip + 16 <= downwellDisplayHeight
                            ? 16 : downwellDisplayHeight - strip;
                        // First materialize any newly dirty rows; the second
                        // read exercises the cache-transfer/fresh-render oracle.
                        drawMainWorldSpan(&probe, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(), generatedDecorations,
                            liveBuilder.decorationCount(), &port, x, worldViewY + strip, 0, count);
                        drawMainWorldSpan(&probe, rooms, mainRoom, builders,
                            fragments, liveBuilder.fragmentCount(), generatedDecorations,
                            liveBuilder.decorationCount(), &port, x, worldViewY + strip, 0, count);
                    }
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                // Verify reserved art survived staging, buffer rotations and
                // the side-room cache probes above. Runs only in this verifier.
                // Exercise the actual bank replacement even though ordinary
                // level 1-1 intentionally does not generate a shop.
                if (!selectOverlayAssetBank(true)) ++gDownwellResultsAssetFailures;
                else {
                    for (ULONG byte = 0; byte < gDownwellShopAssetBytes; ++byte)
                        if (gDownwellShopPack[byte] != amiga_shop_pack[byte])
                            ++gDownwellResultsAssetFailures;
                    const UBYTE* retained = gDownwellShopPack;
                    if (!selectOverlayAssetBank(true) || retained != gDownwellShopPack)
                        ++gDownwellResultsAssetFailures;
                }
                if (!selectOverlayAssetBank(false)) ++gDownwellResultsAssetFailures;
                if (!gDownwellResultsPack || gDownwellResultsAssetBytes !=
                        (ULONG)(amiga_results_pack_end - amiga_results_pack)) {
                    ++gDownwellResultsAssetFailures;
                } else {
                    for (ULONG byte = 0; byte < gDownwellResultsAssetBytes; ++byte)
                        if (gDownwellResultsPack[byte] != amiga_results_pack[byte])
                            ++gDownwellResultsAssetFailures;
                    for (ULONG byte = 0; byte < (ULONG)(amiga_hud_pack_end - amiga_hud_pack); ++byte)
                        if (activeHudPack[byte] != amiga_hud_pack[byte])
                            ++gDownwellResultsAssetFailures;
                }
                // Exercise the real Chip/blitter overlay against the scalar
                // compositor without changing this gameplay replay's death state.
                {
                AmigaVerificationScratch scratch;
                UBYTE* savedHud=scratch.data<2*downwellLayerBytes>();
                UBYTE* expectedHud=savedHud+downwellLayerBytes;
                AmigaHardware::blitterDrain();
                for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                    savedHud[byte] = expectedHud[byte] = renderer.hud[byte];
                AmigaRenderer resultsOracle = renderer;
                resultsOracle.hud = resultsOracle.target = expectedHud;
                // An accidental clip/target switch must not write real terrain
                // in either renderer. Also require observable HUD mutations.
                ULONG gameHashBefore = 2166136261UL;
                for (unsigned byte = 0; byte < downwellGameRingPlaneBytes * 2; ++byte)
                    gameHashBefore = (gameHashBefore ^ renderer.game[byte]) * 16777619UL;
                resultsOracle.blitterEnabled = 0;
                if (!selectOverlayAssetBank(true)) ++gDownwellResultsAssetFailures;
                else {
                    static AmigaShopMenu shopActual, shopOracle;
                    shopActual.reset(); shopOracle.reset();
                    alignas(downwell::ShopWorld::Instance) static UBYTE
                        shopFixtureStorage[sizeof(downwell::ShopWorld::Instance)];
                    auto& fixture = *new (shopFixtureStorage) downwell::ShopWorld::Instance;
                    fixture.present = fixture.menu.active = true;
                    fixture.menu.menuState = 1;
                    fixture.x = 240; fixture.y = 240;
                    fixture.stock.items[0] = {100,1,300};
                    composeDeathFlash(&renderer, false);
                    for (unsigned phase = 0; phase < 7; ++phase) {
                        fixture.menu.cursorPhaseQuarters = phase < 4 ? phase * 4 : 0;
                        if (phase == 1) fixture.menu.cursorWobbleX = 5;
                        if (phase == 2) fixture.stock.items[0].remaining = 0;
                        if (phase == 3) fixture.menu.cursor = 0;
                        if (phase == 6) fixture.menu.active = false;
                        unsigned streak = phase == 4 ? 12 : phase == 5 ? 0 : 3;
                        // Actual retained overlay experiences real external HUD
                        // clears; the scalar oracle starts fresh every phase.
                        redrawStreak(&renderer, streak);
                        shopActual.invalidateStreak(streakYOffset,streakYOffset);
                        composeDeathFlash(&resultsOracle, false);
                        shopOracle.reset();
                        shopOracle.invalidateStreak(streakYOffset,streakYOffset);
                        const auto foreground = [](AmigaRenderer* target, void* context) {
                            composeStreak(target, plainStreakView(*static_cast<unsigned*>(context)));
                        };
                        if (!shopActual.draw(&renderer, gDownwellShopPack,
                                gDownwellShopPack + gDownwellShopAssetBytes, &fixture, 160, 34, 200, foreground, &streak) ||
                            !shopOracle.draw(&resultsOracle, gDownwellShopPack,
                                gDownwellShopPack + gDownwellShopAssetBytes, &fixture, 160, 34, 200, foreground, &streak))
                            ++gDownwellResultsAssetFailures;
                        AmigaHardware::blitterDrain();
                        for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                            if (renderer.hud[byte] != expectedHud[byte]) ++gDownwellResultsAssetFailures;
                        ++gDownwellShopOverlayChecks;
                    }
                    for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                        renderer.hud[byte] = expectedHud[byte] = savedHud[byte];
                }
                if (!selectOverlayAssetBank(false)) ++gDownwellResultsAssetFailures;
                // Verify both flash edges, not only the cleared final state.
                for (unsigned flash = 0; flash < 2; ++flash) {
                    composeDeathFlash(&renderer, flash == 0);
                    amigaRendererClearClipRect(&resultsOracle);
                    amigaRendererFillRect(&resultsOracle, downwellPlayfieldX, 0, 160, 284,
                        flash == 0 ? 1 : 0);
                    AmigaHardware::blitterDrain();
                    for (unsigned byte = 0; byte < downwellLayerBytes; ++byte)
                        if (renderer.hud[byte] != expectedHud[byte]) ++gDownwellResultsBackdropFailures;
                }
                composeResultsBackdrop(&resultsOracle, 4, 160, 34);
                composeResultsBackdrop(&renderer, 4, 160, 34);
                downwell::DeathMenuSequence rowFixture;
                for (unsigned row = 0; row < 5; ++row) rowFixture.visible[row] = true;
                rowFixture.rowOffset[1] = 3;
                rowFixture.rowOffset[2] = 2;
                composeResultsRows(&resultsOracle, rowFixture, 84, 10, 3699, 25, {2,3});
                composeResultsRows(&renderer, rowFixture, 84, 10, 3699, 25, {2,3});
                downwell::DeathResultsProgressBar barFixture;
                barFixture.visible = true; barFixture.length = 47; barFixture.separator = 24;
                downwell::DeathResults labelFixture;
                labelFixture.open(true, 500, 500);
                labelFixture.menu.visible[5] = true;
                labelFixture.frame.displayedTotal = 500;
                composeResultsProgress(&resultsOracle, barFixture, labelFixture);
                composeResultsProgress(&renderer, barFixture, labelFixture);
                rowFixture.sequenceOver = true;
                // The real HUD visits all choices before the final frame;
                // the scalar oracle draws only the final state on fresh backdrop.
                for (int cursor = 0; cursor < 3; ++cursor) {
                    rowFixture.cursor = cursor;
                    for (int bounce = 0; bounce <= 12; bounce += 4) {
                        rowFixture.cursorBounceTenths = bounce;
                        composeResultsChoices(&renderer, rowFixture, 160, 34);
                    }
                }
                rowFixture.goalPassed = true;
                composeResultsChoices(&renderer, rowFixture, 160, 34);
                rowFixture.goalPassed = false;
                rowFixture.cursor = 2;
                rowFixture.cursorBounceTenths = 12;
                composeResultsChoices(&resultsOracle, rowFixture, 160, 34);
                composeResultsChoices(&renderer, rowFixture, 160, 34);
                redrawAmmo(&resultsOracle, 12, 15);
                redrawAmmo(&renderer, 12, 15);
                AmigaHardware::blitterDrain();
                unsigned changedHudBytes = 0;
                ULONG gameHashAfter = 2166136261UL;
                for (unsigned byte = 0; byte < downwellGameRingPlaneBytes * 2; ++byte)
                    gameHashAfter = (gameHashAfter ^ renderer.game[byte]) * 16777619UL;
                if (gameHashAfter != gameHashBefore) ++gDownwellResultsBackdropFailures;
                for (unsigned byte = 0; byte < downwellLayerBytes; ++byte) {
                    if (renderer.hud[byte] != savedHud[byte]) ++changedHudBytes;
                    if (renderer.hud[byte] != expectedHud[byte]) ++gDownwellResultsBackdropFailures;
                    renderer.hud[byte] = savedHud[byte];
                }
                if (changedHudBytes < 100) ++gDownwellResultsBackdropFailures;
                ++gDownwellResultsBackdropChecks;
                }
                // Run the same deferred consumable code on the actual 68000,
                // isolated from the 600-update world's replay and terrain.
                alignas(PlayerMotion) static UBYTE itemPlayerStorage[sizeof(PlayerMotion)];
                auto& itemPlayer = *new (itemPlayerStorage) PlayerMotion(false);
                static downwell::Consumables itemQueue;
                itemQueue = {};
                downwell::CurrencyAccount itemCurrency;
                downwell::HealthReserve itemHealth;
                int hp = 4;
                for (unsigned id = 100; id <= 105; ++id) itemQueue.grant(id, itemCurrency);
                itemQueue.grant(100, itemCurrency);
                itemQueue.beginStep(itemPlayer, hp, itemHealth);
                ++gDownwellConsumableChecks;
                if (hp != 6 || itemHealth.maximum != 6 || itemHealth.pieces != 0 ||
                    itemPlayer.ammoCapacity() != 12 || itemPlayer.ammo() != 12 ||
                    itemQueue.effects.count != 6 || itemQueue.pending[0] != 1)
                    ++gDownwellConsumableFailures;
                itemQueue.beginStep(itemPlayer, hp, itemHealth);
                ++gDownwellConsumableChecks;
                if (itemHealth.pieces != 1 || itemQueue.effects.count != 1 || itemQueue.pending[0])
                    ++gDownwellConsumableFailures;
                itemQueue.beginStep(itemPlayer, hp, itemHealth);
                ++gDownwellConsumableChecks;
                if (itemQueue.effects.count != 0) ++gDownwellConsumableFailures;
                downwellGameplayLongRunReady();
            }
            static bool ammoFrameReported = false;
            if (gDownwellGemHighCapture == 2) {
                gDownwellGemHighStreak = port.gemStreak();
                unsigned visible = 0;
                for (unsigned i = 0; i < AmigaPortRuntime::bulletCapacity; ++i) {
                    const auto& bullet = port.bulletAt(i);
                    const int x = 80 + bullet.x - port.cameraOffsetX();
                    const int y = gameplayPlayerStartY + bullet.y - port.cameraOffsetY();
                    if (bullet.active && bullet.gemHigh && bullet.animation.frame() != 0 &&
                            x >= 8 && x < 152 && y >= 8 && y < 276)
                        ++visible;
                }
                if (visible) {
                    gDownwellGemHighVisibleShots = visible;
                    gDownwellGemHighCapture = 3;
                    exposeGameplayFrame(memory, gameDisplay[back], back, port);
                    downwellGameplayGemHighFrameReady();
                }
            }
            static bool rechargeFrameReported = false;
            if (!rechargeFrameReported && port.ammoMeterOffset() == 3) {
                rechargeFrameReported = true;
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                downwellGameplayRechargeFrameReady();
            }
            if (!ammoFrameReported && port.player().ammo() < port.player().ammoCapacity() && port.ammoHudOffset() == 2) {
                ammoFrameReported = true;
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                downwellGameplayAmmoFrameReady();
            }
            if (!sceneGameplayLandedReported && port.player().grounded()) {
                sceneGameplayLandedReported = true;
                sceneGameplayLandedCameraY = port.cameraOffsetY();
                exposeGameplayFrame(memory, gameDisplay[back], back, port);
                downwellGameplayFrameReady();
                downwellSceneGameplayLanded();
            } else if (sceneGameplayLandedReported &&
                       !sceneGameplayActiveReported &&
                       port.cameraOffsetY() >=
                           sceneGameplayLandedCameraY + 256) {
                bool awakeEnemy = false;
                for (unsigned enemy = 0; enemy < port.enemyCount(); ++enemy)
                    awakeEnemy = awakeEnemy || port.enemyAt(enemy).awake;
                if (awakeEnemy) {
                    sceneGameplayActiveReported = true;
                    exposeGameplayFrame(memory, gameDisplay[back], back, port);
                    downwellGameplayActiveFrameReady();
                    downwellSceneGameplayActive();
                }
            }
#endif
