// DOWNWELL_TERRAIN_BATCH_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 10
            // Full native comparison against the retained painter, which also
            // checks its own output against the scalar compositor in this build.
            static UWORD oracle[284*22];
            for(int y=0;y<lastY;y+=284){
                const int height=lastY-y<284?lastY-y:284;
                bool equal=AmigaTerrainPainter::paintCached(oracle,renderer,rooms,room,terrainCollision.index,
                    terrainTiles,decorations,decorationCount,runtime,activeGamePack,activeGamePackEnd,
                    int(column)*160,y,0,height);
                for(int row=0;row<height&&equal;++row){
                    UWORD scratch[22];
                    const UWORD* actual=terrainRaster.readRow(column,y+row,scratch);
                    for(int word=0;word<22&&equal;++word)
                        equal=oracle[row*22+word]==actual[word];
                }
                if(!equal){
                    terrainPaintSucceeded=false;downwellTerrainBatchMismatch();return;
                }
                ++gDownwellInitialTerrainChecks;
            }
#endif
