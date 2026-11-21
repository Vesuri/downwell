// DOWNWELL_EXIT_WIPE_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 34
            if (exitMask) {
                ++gDownwellExitWipeFrames;
                const auto& list = gameplayScene.copper(back);
                const UWORD* words = list.data();
                if (words[list.moveOffset(0x100) + 1] != 0x6401) ++gDownwellExitWipeFailures;
                for (unsigned reg = 0xf0; reg <= 0xf4; reg += 4) {
                    const unsigned at = list.moveOffset(reg);
                    const ULONG ptr = (ULONG(words[at + 1]) << 16) | words[at + 3];
                    if (ptr != ULONG(exitMask + downwellFetchCropBytes)) ++gDownwellExitWipeFailures;
                }
                if (gDownwellExitWipeFrames == 1)
                    for (unsigned i = 0; i < AmigaExitMask::bytes; ++i)
                        if (overlayAssetStorage[i] != amiga_exit_mask[i]) ++gDownwellExitWipeFailures;
                if (exitMask == overlayAssetStorage) {
                    platform.waitVblank();
                    downwellExitWipeReady();
                }
            }
#endif
