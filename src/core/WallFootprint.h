#ifndef DOWNWELL_CORE_WALL_FOOTPRINT_H
#define DOWNWELL_CORE_WALL_FOOTPRINT_H
namespace downwell {
// Serialized dimensions/origins are pinned by verify-terrain-create.py.
// -1 is the presentation observer's hidden-wall sentinel.
inline int wallRadius(int sprite){
    switch(sprite){
        case -1:return 0;
        case 70:return 16;
        case 51:case 52:case 53:case 55:case 57:case 72:case 74:case 76:return 8;
        default:return -1;
    }
}
// Half-open row intervals. Subtract only ordered unsigned coordinates so
// off-screen instances at the signed coordinate limits cannot overflow.
inline bool wallIntersectsRows(int sprite,int y,int first,int end){
    const int radius=wallRadius(sprite);
    if(radius==0||first>=end)return false;
    if(radius<0)return true; // Unknown artwork must not be silently culled.
    if(y<first&&unsigned(first)-unsigned(y)>=unsigned(radius))return false;
    if(y>=end&&unsigned(y)-unsigned(end)>=unsigned(radius))return false;
    return true;
}
}
#endif
