// DOWNWELL_CAMERA_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 26
            // Bind through the world: step() rebinds the player's callbacks.
            port.configureLandingResolver([](void*, int, int) { return 0; }, nullptr);
            port.configureSolidResolver([](void*, int, int) { return false; }, nullptr);
            if (!port.tick()) {
                // Exercise cold staging, not just transfers from a fully
                // prepared raster. Production invalidation follows generation.
                for (unsigned y=0; y<terrainRasterRows; ++y) {
                    terrainRaster.setDirty(0,y,true);
                    terrainRaster.setDirty(2,y,true);
                }
            }
            if (!port.tick() && (!port.registerSideCameraLocker(385, 256) ||
                                 !port.registerSideCameraLocker(95, 288))) return 20;
            const bool wasSideLocked = port.cameraSideLocked();
#endif
