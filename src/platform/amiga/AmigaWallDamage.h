#ifndef DOWNWELL_AMIGA_WALL_DAMAGE_H
#define DOWNWELL_AMIGA_WALL_DAMAGE_H
#include "core/WallFootprint.h"
// Sparse world-row debts, independent of camera position and ring origin.
// Publish invalidation before camera/staging decisions; acknowledge a band only
// after its visible repair succeeds. Callbacks must not mutate this queue.
template<unsigned Capacity=2048> class AmigaWallDamage {
    static_assert(Capacity>0,"Wall damage needs bounded storage");
    struct Band {int first,end;};
    Band bands_[Capacity]{};unsigned count_=0;bool good_=true;
public:
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    void clear(){count_=0;good_=true;}
    bool changed(int,int y,int before,int after){
        const int a=downwell::wallRadius(before),b=downwell::wallRadius(after),r=a>b?a:b;
        if(!good_||a<0||b<0||y<(-2147483647-1)+r||y>2147483647-r)return good_=false;
        return !r||mark(y-r,y+r);
    }
    bool mark(int first,int end){
        if(!good_||first>end)return good_=false;
        if(first==end)return true;
        unsigned begin=0;while(begin<count_&&bands_[begin].end<first)++begin;
        unsigned last=begin;
        while(last<count_&&bands_[last].first<=end){
            if(bands_[last].first<first)first=bands_[last].first;
            if(bands_[last].end>end)end=bands_[last].end;
            ++last;
        }
        if(begin==last&&count_==Capacity)return good_=false;
        const unsigned removed=last-begin;
        if(!removed)for(unsigned i=count_;i>begin;--i)bands_[i]=bands_[i-1];
        else for(unsigned i=last;i<count_;++i)bands_[begin+1+i-last]=bands_[i];
        count_=count_+1-removed;bands_[begin]={first,end};return true;
    }
    template<class Visit> bool visit(Visit visit)const{
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i)if(!visit(bands_[i].first,bands_[i].end))return false;
        return true;
    }
    template<class Repair> bool repair(Repair repair){
        if(!good_)return false;
        unsigned done=0;while(done<count_&&repair(bands_[done].first,bands_[done].end))++done;
        for(unsigned i=done;i<count_;++i)bands_[i-done]=bands_[i];
        count_-=done;return count_==0;
    }
};
#endif
