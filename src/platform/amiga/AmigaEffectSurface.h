#ifndef DOWNWELL_AMIGA_EFFECT_SURFACE_H
#define DOWNWELL_AMIGA_EFFECT_SURFACE_H
#include "core/PaletteEffectSurface.h"
#include "core/EffectMaskAssets.h"
class Bitmap;
class AmigaGameplayRows;
class AmigaEffectSurface {
public:
    // One guard word on each side permits the framework's shifted blits.
    static constexpr unsigned rowBytes=24,planeBytes=rowBytes*284,bytes=planeBytes*3;
#ifdef DOWNWELL_REFERENCE_EFFECT_COMPOSITION
    static constexpr bool interleaved=false;
#else
    static constexpr bool interleaved=true;
#endif
    static bool bind(downwell::PaletteEffectSurface<>& surface,void* chipStorage,
                     const unsigned char* stamps,unsigned stampBytes,
                     const downwell::EffectMaskAssets& assets);
    static bool verify(const downwell::PaletteEffectSurface<>& surface);
    static bool fixtures(downwell::PaletteEffectSurface<>& surface,const downwell::EffectMaskAssets& assets);
    static bool publish(const downwell::PaletteEffectSurface<>& surface,Bitmap& display,
                        unsigned buffer,unsigned origin,AmigaGameplayRows& rows);
};
#endif
