// DOWNWELL_SHOP_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 29
            if (port.shops().allocatedCount && gDownwellShopPack) {
                alignas(2) static UBYTE expected[downwellLayerBytes];
                AmigaHardware::blitterDrain();
                for (unsigned i = 0; i < downwellLayerBytes; ++i) expected[i] = renderer.hud[i];
                AmigaRenderer oracle = renderer;
                oracle.hud = oracle.target = expected;
                oracle.blitterEnabled = 0;
                amigaRendererClearClipRect(&oracle);
                amigaRendererFillRect(&oracle, downwellPlayfieldX, 0, 160, 284, 0);
                static AmigaShopMenu fresh;
                fresh.reset();
                if (!fresh.draw(&oracle, gDownwellShopPack,
                        gDownwellShopPack + gDownwellShopAssetBytes, activeShop,
                        mainViewX + port.cameraSteadyOffsetX(), mainInitialViewY + port.cameraOffsetY(),
                        port.currency())) ++gDownwellShopLiveFailures;
                composeStreak(&oracle, streakView);
                for (unsigned i = 0; i < downwellLayerBytes; ++i)
                    if (expected[i] != renderer.hud[i]) ++gDownwellShopLivePixelFailures;
                ++gDownwellShopLiveFrames;
            }
#endif
