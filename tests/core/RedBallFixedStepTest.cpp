#include "core/RedBallFixedStep.h"
#include "core/RedBallStep.h"
#include "core/WhiteJellyStep.h"
#include "core/CurrencyCollisionBounds.h"
#include "core/RunnerDirectionValue.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// The compact world's Q16 red balls (118 and neon 173) against RedBallStep,
// their VM form, driven side by side through the same walls, target path,
// view, hits, enemy contacts and effect alarms to their deaths: the RNG
// stays in step and the published pixels within one of each other.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Red ball fixed check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
using F=downwell::RunnerImageValue;
using Q=downwell::Fixed16;
namespace {
constexpr int mask=153;
bool solid(int x,int y){
    if(x<32||x>=288)return true;
    const int cx=x>>4,cy=y>>4;
    return (cy%9==4&&cx%5!=0)||(cy%13==7&&cx>=9&&cx<=12);
}
bool overlaps(const downwell::ContactBounds& b){
    for(int y=b.top;y<b.bottom;++y)for(int x=b.left;x<b.right;++x)if(solid(x,y))return true;
    return false;
}
struct World {
    int targetX=160,targetY=200;bool visible=true;
    unsigned meetings=0,hits=0,effects=0,alarms=0,flashes=0,deaths=0;
    int sprite=0,xscale=1,alarm0=-1;
};
// The reference's movingFx/flash arguments, kept to compare with the Q16 ones.
struct Effect {double x,y,rate,angle,speed;};
struct Ops {
    World& world;Effect effects[16]{};unsigned count=0;
    // VM form.
    bool playerX(N& v){v=N::fromInteger(world.targetX);return true;}
    bool playerY(N& v){v=N::fromInteger(world.targetY);return true;}
    bool enemyPlayerX(N& v){return playerX(v);}
    bool enemyPlayerY(N& v){return playerY(v);}
    bool pointDirection(N x,N y,N tx,N ty,N& v){
        Q ax,ay,bx,by;
        if(!Q::fromBinary64(x,ax)||!Q::fromBinary64(y,ay)||!Q::fromBinary64(tx,bx)||!Q::fromBinary64(ty,by))return false;
        v=Q::raw(downwell::FixedTrig::pointDirection(bx.v-ax.v,by.v-ay.v)).binary64();return true;
    }
    bool direction(N wanted,N& accepted){
        F value;if(!downwell::RunnerDirectionValue::normalize(wanted,value))return false;
        accepted=value.promoted();return true;
    }
    bool lengthdir(bool vertical,N speed,N angle,N& v){return downwell::WhiteJellyStep::movementComponent(vertical,speed,angle,v);}
    bool meeting(N x,N y,int object,bool& hit){
        downwell::ContactBounds b;
        if(object!=57||!downwell::CurrencyCollisionBounds::calculateMask(mask,x,y,b))return false;
        hit=overlaps(b);return true;
    }
    static double real(N v){Q q;checkAt(Q::fromBinary64(v,q),__LINE__);return q.v/65536.;}
    bool movingFx(N x,N y,int sprite,N rate,N angle,N speed){
        check(sprite==704&&count<16);effects[count++]={real(x),real(y),real(rate),real(angle),real(speed)};return true;
    }
    bool flash(N x,N y,int a,int b,int c){check(a==1&&!b&&!c);effects[count++]={real(x),real(y),0,0,0};return true;}
    bool effectAlarmReset(N value){check(real(value)==10);world.alarm0=10;return true;}
    // Q16 form.
    bool playerQ16(bool vertical,std::int32_t& v){v=(vertical?world.targetY:world.targetX)*65536;return true;}
    bool enemyPlayerQ16(bool vertical,std::int32_t& v){return playerQ16(vertical,v);}
    bool meetingQ16(std::int32_t x,std::int32_t y,int object,bool& hit){
        downwell::ContactBounds b;
        if(object!=57||!downwell::CurrencyCollisionBounds::calculateMaskQ16(mask,x,y,b))return false;
        ++world.meetings;hit=overlaps(b);world.hits+=hit;return true;
    }
    bool movingFxQ16(Q x,Q y,int sprite,F rate,Q angle,Q speed){
        check(sprite==704&&count<16);Q r;check(Q::fromBinary32(rate.bits,r));
        effects[count++]={x.v/65536.,y.v/65536.,r.v/65536.,angle.v/65536.,speed.v/65536.};return true;
    }
    bool flashQ16(Q x,Q y){effects[count++]={x.v/65536.,y.v/65536.,0,0,0};return true;}
    bool effectAlarmReset(int ticks){check(ticks==10);world.alarm0=10;return true;}
    // Shared builtins.
    bool position(bool,F value,F& accepted){accepted=value;return true;}
    bool imageSpeed(F value,F& accepted){accepted=value;return true;}
    bool alarm(int,int){return true;}
    bool imageIndex(int){return true;}
    bool sound(int,int,int,int,const char*){return true;}
    bool inView(bool& v){v=world.visible;return true;}
    bool xscale(int sign){world.xscale=sign;return true;}
    bool enemyDeath(){++world.deaths;return true;}
    bool currency(int){return true;}
    bool destroy(){return true;}
    bool sprite(int id){world.sprite=id;return true;}
};
int pixel(F v){std::int32_t n;checkAt(v.promoted().roundPixel(n),__LINE__);return int(n);}
double gap(double a,double b){return a>b?a-b:b-a;}
void sameEffects(const Ops& a,const Ops& b){
    check(a.count==b.count);
    // Positions are xx/yy (within the step's drift); samples are the nearest
    // Q16 of the VM's, and the rate is narrowed to binary32 once.
    for(unsigned i=0;i<a.count;++i){
        const Effect& e=a.effects[i];const Effect& f=b.effects[i];
        check(gap(e.x,f.x)<=1&&gap(e.y,f.y)<=1&&gap(e.rate,f.rate)<=1/65536.&&
              gap(e.angle,f.angle)<=1/65536.&&gap(e.speed,f.speed)<=1/65536.);
    }
}
}
int main(){
    unsigned steps=0,pixelDifferences=0,headingDifferences=0,deaths=0;long worst=0;
    World totals;
    for(bool neon:{false,true})for(unsigned seed=0;seed<64;++seed){
        World referenceWorld,fixedWorld;
        downwell::GameMakerRandom referenceRandom(seed),fixedRandom(seed),path(seed+1000);
        downwell::RedBallStep reference;downwell::RedBallFixedStep fixed;
        reference.neon=fixed.neon=neon;
        Ops referenceOps{referenceWorld},fixedOps{fixedWorld};
        const int sx=48+16*int(path.irandomSmall(14)),sy=16*int(path.irandomSmall(20));
        F px,py;check(px.assignInteger(sx)&&py.assignInteger(sy));
        check(reference.create(px,py,referenceRandom,referenceOps)&&fixed.create(sx,sy,fixedRandom,fixedOps));
        check(pixel(reference.x)==fixed.pixelX&&pixel(reference.y)==fixed.pixelY);
        for(unsigned tick=0;tick<700;++tick){
            World* worlds[]={&referenceWorld,&fixedWorld};
            if(tick%40==0){
                const int tx=48+int(path.irandomSmall(224)),ty=sy-64+int(path.irandomSmall(400));
                const bool visible=path.irandomSmall(4)!=0;
                for(World* w:worlds){w->targetX=tx;w->targetY=ty;w->visible=visible;}
            }
            if(tick%97==50){
                const int degrees=int(path.irandomSmall(359));
                reference.hit=fixed.hit=true;reference.hitDirection=N::fromInteger(degrees);
                fixed.hitDirection16=degrees*65536;
                reference.health-=7;fixed.health-=7;
            }
            if(tick%97==55){check(reference.hitStunAlarm()&&fixed.hitStunAlarm());}
            if(tick%61==30){check(reference.enemyContact(referenceRandom,referenceOps)&&fixed.enemyContact(fixedRandom,fixedOps));}
            referenceOps.count=fixedOps.count=0;
            if(tick%10==5){
                const bool stopped=tick%50==45;
                check(reference.effectAlarm(stopped,referenceRandom,referenceOps)&&fixed.effectAlarm(stopped,fixedRandom,fixedOps));
            }
            const bool stopped=tick%150>=140;
            const bool alive=reference.alive;
            check(reference.step(stopped,referenceRandom,referenceOps)&&fixed.step(stopped,fixedRandom,fixedOps));
            sameEffects(referenceOps,fixedOps);
            check(referenceRandom.drawCount()==fixedRandom.drawCount());
            check(reference.alive==fixed.alive&&reference.hitStun==fixed.hitStun&&reference.health==fixed.health);
            check(referenceWorld.sprite==fixedWorld.sprite&&referenceWorld.xscale==fixedWorld.xscale&&
                  referenceWorld.alarm0==fixedWorld.alarm0);
            {
                Q ex,ey;check(Q::fromBinary64(reference.xx,ex)&&Q::fromBinary64(reference.yy,ey));
                const long dx=std::labs(long(fixed.x16)-ex.v),dy=std::labs(long(fixed.y16)-ey.v);
                if(dx>worst)worst=dx;
                if(dy>worst)worst=dy;
            }
            const int rx=pixel(reference.x),ry=pixel(reference.y);
            check(std::abs(rx-fixed.pixelX)<=1&&std::abs(ry-fixed.pixelY)<=1);
            pixelDifferences+=(rx!=fixed.pixelX)+(ry!=fixed.pixelY);
            Q heading;check(Q::fromBinary64(reference.direction,heading));
            headingDifferences+=std::labs(long(heading.v)-fixed.direction16)>65536;
            // The eye sector follows playerDir.
            struct DrawOps {
                bool shade(){return true;}
                bool drawSelf(){return true;}
                bool imageIndex(N& v){v={};return true;}
                bool drawSpriteExt(int,N,N,N,int,int,int,unsigned,int){return true;}
            } draw;
            const bool drawn=reference.draw(draw);
            check(drawn==fixed.updateEye());
            if(drawn)check(reference.eye==fixed.eye&&reference.eyeX==fixed.eyeX&&reference.shotDirection==fixed.shotDirection);
            ++steps;
            if(alive&&!reference.alive){++deaths;break;}
        }
        totals.meetings+=fixedWorld.meetings;totals.hits+=fixedWorld.hits;
    }
    std::printf("Red ball Q16: %u steps, %u deaths, %u meetings (%u walls), positions within %.4f px of RedBallStep, "
                "%u of %u published pixels differ by one, %u headings over a degree apart\n",
                steps,deaths,totals.meetings,totals.hits,worst/65536.0,pixelDifferences,2*steps,headingDifferences);
    check(deaths==128&&totals.hits*20>totals.meetings&&worst<=1024&&pixelDifferences*100<2*steps&&headingDifferences*100<steps);
    // The direction setter's numeric part against RunnerDirectionValue.
    for(std::int32_t v=-800*65536;v<=800*65536;v+=4093){
        F expected;check(downwell::RunnerDirectionValue::normalize(Q::raw(v).binary64(),expected));
        Q e;check(Q::fromBinary32(expected.bits,e));
        check(downwell::SteeringFixedState::normalizeDirection(v)==e.v);
    }
    std::puts("Red ball Q16: direction setter equals RunnerDirectionValue on -800..800 degrees");
}
