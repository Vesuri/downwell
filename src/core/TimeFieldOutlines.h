#ifndef DOWNWELL_CORE_TIME_FIELD_OUTLINES_H
#define DOWNWELL_CORE_TIME_FIELD_OUTLINES_H
#include "GoalFieldOutline.h"
#include "OrdinaryTimeFields.h"
namespace downwell {
// Compact immutable presentation snapshot. Store only visible circle inputs,
// not 72 vertices for every world field. No Draw RNG is consumed here.
struct TimeFieldOutlines {
    struct Circle { int x, y, radius; } circles[66] = {};
    unsigned count = 0;
    bool visible = false;
    void set(const OrdinaryTimeFields<64>& fields, const GoalTimeField& goal,
             bool goalPresent, unsigned goalBoundary, int viewX, int viewY,
             int screenX = 0,const GoalTimeField* start=nullptr) {
        count = 0; visible = false;
        if (viewX + 80 <= 160 || viewX + 80 >= 320) return;
        auto append = [&](int x, int y, HudReal radius, int minimum) {
            const int extent = minimum + 4;
            if (y + extent < viewY || y - extent >= viewY + 284 ||
                x + extent < viewX || x - extent >= viewX + 160) return;
            radius.exponent += 8;
            const int base = static_cast<int>(radius.floor());
            if (base < minimum * 256 || base >= (minimum + 2) * 256) return;
            circles[count++] = {x - viewX + screenX, y - viewY, base};
        };
        for (unsigned i = fields.size(); i; --i) {
            if (goalPresent && i == goalBoundary) append(goal.x, goal.y, goal.radius, 128);
            const auto& field = *fields.at(i - 1);
            append(field.x, field.y, field.radius, 24);
        }
        if (goalPresent && goalBoundary == 0) append(goal.x, goal.y, goal.radius, 128);
        if(start)append(start->x,start->y,start->radius,128);
        visible = count != 0;
    }
    bool same(const TimeFieldOutlines& other) const {
        if (count != other.count) return false;
        for (unsigned i = 0; i < count; ++i)
            if (circles[i].x != other.circles[i].x || circles[i].y != other.circles[i].y ||
                circles[i].radius != other.circles[i].radius) return false;
        return true;
    }
    template<class Plot> void draw(Plot plot) const {
        GoalFieldOutline outline;
        for (unsigned i = 0; i < count; ++i) {
            outline.setRasterCircle(circles[i].x, circles[i].y, circles[i].radius);
            outline.draw(plot);
        }
    }
    template<class Plot> void drawClipped(int left,int top,int right,int bottom,Plot plot) const {
        GoalFieldOutline outline;
        for (unsigned i = 0; i < count; ++i) {
            outline.setRasterCircle(circles[i].x, circles[i].y, circles[i].radius);
            outline.drawClipped(left,top,right,bottom,plot);
        }
    }
    template<class Span> void drawClippedSpans(int left,int top,int right,int bottom,Span span) const {
        GoalFieldOutline outline;
        for(unsigned i=0;i<count;++i){
            outline.setRasterCircle(circles[i].x,circles[i].y,circles[i].radius);
            outline.drawClippedSpans(left,top,right,bottom,span);
        }
    }
};
}
#endif
