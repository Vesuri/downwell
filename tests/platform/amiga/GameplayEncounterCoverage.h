#ifndef DOWNWELL_TEST_GAMEPLAY_ENCOUNTER_COVERAGE_H
#define DOWNWELL_TEST_GAMEPLAY_ENCOUNTER_COVERAGE_H
#include <array>
#include <cstdint>
#include <iostream>

// Host-only census, not instrumentation in the timed native frame. Count live
// actor centres inside the source 160x284 camera, not allocated room slots or
// pixels (occlusion and sprite extents are deliberately not inferred).
struct GameplayEncounterCoverage {
    using Counts=std::array<unsigned,5>; // enemies, bullets, currency, smoke, debris
    Counts maxima{},peak{};
    unsigned peakTotal=0,peakFrame=0,denseFrames=0,longestDenseRun=0,denseRun=0;
    template<class World> bool sample(const World& game,unsigned frame) {
        Counts counts{};
        const int left=160+game.cameraOffsetX(),top=34+game.cameraOffsetY();
        const auto inside=[&](int x,int y){return x>=left&&x<left+160&&y>=top&&y<top+284;};
        for(unsigned i=0;i<game.enemyCount();++i){const auto& e=game.enemyAt(i);
            if(e.active&&!e.destroyRequested&&inside(240+e.x,176+e.y))++counts[0];}
        for(unsigned i=0;i<game.bulletSlotEnd();++i){const auto& b=game.bulletAt(i);
            if(b.active&&!b.destroyRequested&&inside(240+b.x,176+b.y))++counts[1];}
        for(unsigned i=0;i<game.gemSlotEnd();++i){const auto& g=game.gemAt(i);
            if(g.active&&inside(240+g.x,176+g.y))++counts[2];}
        bool okay=true;
        const auto effect=[&](const auto& e,unsigned family){
            if(e.destroyRequested)return;
            std::int32_t x,y;
            if(!e.x.promoted().roundPixel(x)||!e.y.promoted().roundPixel(y)){okay=false;return;}
            if(inside(x,y))++counts[family];
        };
        game.smokeEffects().visitNewest([&](auto,const auto& e){effect(e,3);});
        if(!game.debrisEffects().visitNewest([&](auto,const auto& e){
            if(e.flash<=0)effect(e,4);
        }))okay=false;
        unsigned total=0;
        for(unsigned i=0;i<counts.size();++i){
            total+=counts[i];if(counts[i]>maxima[i])maxima[i]=counts[i];
        }
        if(total>peakTotal){peakTotal=total;peakFrame=frame;peak=counts;}
        // A fixed reporting threshold, not an assertion that this alone meets
        // the unfinished dense-encounter coverage requirement.
        if(total>=24){++denseFrames;++denseRun;if(denseRun>longestDenseRun)longestDenseRun=denseRun;}
        else denseRun=0;
        return okay;
    }
    void report()const{
        std::cout<<"Encounter centres peak_frame="<<peakFrame<<" peak_total="<<peakTotal
            <<" frames_ge24="<<denseFrames<<" longest_ge24="<<longestDenseRun;
        const char* names[]={"enemies","bullets","currency","smoke","debris"};
        for(unsigned i=0;i<peak.size();++i)std::cout<<" peak_"<<names[i]<<'='<<peak[i]
            <<" max_"<<names[i]<<'='<<maxima[i];
        std::cout<<'\n';
    }
};
#endif
