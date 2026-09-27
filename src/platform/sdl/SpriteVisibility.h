#ifndef DOWNWELL_SDL_SPRITE_VISIBILITY_H
#define DOWNWELL_SDL_SPRITE_VISIBILITY_H
namespace downwell {
// Bounds of the actual trimmed/scaled destination, not the sprite anchor.
// Retain a pixel of slack for backend raster rounding. Rotated sprites take
// the normal SDL path: their unrotated rectangle is not a safe rejection test.
inline bool sdlSpriteOutside(float x,float y,float width,float height,float rotation,
                             int viewportWidth,int viewportHeight) {
    return rotation==0.0F &&
        (x+width < -1.0F || y+height < -1.0F ||
         x > viewportWidth+1.0F || y > viewportHeight+1.0F);
}
}
#endif
