#ifndef DOWNWELL_PROJECTILE_VIEW_INTERIOR_H
#define DOWNWELL_PROJECTILE_VIEW_INTERIOR_H
#include "RunnerImageValue.h"
namespace downwell {
// One rmMain End Step phase has an immutable Q12 camera top and height 284.
// Prove all four source retirement comparisons false without evaluating them
// per bullet. Outside this interior, keep the full source-ordered adapter.
struct ProjectileViewInterior {
    std::int32_t first,last;
    explicit ProjectileViewInterior(std::int32_t topQ12){
        const auto remainder=topQ12%4096;
        const auto floor=topQ12/4096-(remainder<0);
        first=floor+(remainder!=0)-99;
        last=floor+315;
    }
    bool contains(RunnerImageValue y)const{
        const unsigned exponent=(y.bits>>23)&255u;
        if(exponent==255u||(!exponent&&(y.bits&0x7fffffffu)))return false;
        std::int32_t pixel;
        // Rounding changes y by at most 0.5. Admitted y is therefore strictly
        // inside [top-100, top+316] by at least 0.5, far beyond the runner's
        // 1e-5 comparison band; also inside [top-160, top+3000]. The int32 Q12
        // top keeps these bounds finite and exact in binary64. No destruction
        // callback (and hence no callback-driven reread) can occur here.
        return y.roundPixel(pixel)&&pixel>=first&&pixel<=last;
    }
};
}
#endif
