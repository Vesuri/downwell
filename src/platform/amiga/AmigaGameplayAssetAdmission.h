#ifndef DOWNWELL_AMIGA_GAMEPLAY_ASSET_ADMISSION_H
#define DOWNWELL_AMIGA_GAMEPLAY_ASSET_ADMISSION_H
// This is an admission boundary, not the game's progression rules. New
// playable areas need reviewed CPU/DMA dependencies before being admitted.
struct AmigaGameplayAssetAdmission {
    static constexpr bool production(int area){return area==0||area==1;}
    // Explicit pixel/replay oracles exercise the complete exported catalogue.
    static constexpr bool referenceOracle(int area){return area>=0&&area<=5;}
};
#endif
