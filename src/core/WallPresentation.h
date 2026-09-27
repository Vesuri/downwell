#ifndef DOWNWELL_CORE_WALL_PRESENTATION_H
#define DOWNWELL_CORE_WALL_PRESENTATION_H
namespace downwell {
// Transitional legacy callback is only for walls whose End Step has not been
// connected yet. Never recompute an event-selected frame after neighbors die.
template<class Record,class Legacy,class Draw>
bool drawWallPresentation(const Record& r,Legacy legacy,Draw draw){
    if(r.objectId!=60||!r.active||r.destroyRequested)return true;
    if(!r.wall.checked){draw(51,legacy());return true;}
    if(r.spriteId<0||!(r.imageIndex>=0&&r.imageIndex<=15))return false;
    draw(r.spriteId,static_cast<unsigned>(r.imageIndex));return true;
}
}
#endif
