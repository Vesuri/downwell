// Included by GameplayWorld.cpp: the compact owners of the world-one
// upgrades' own objects (W1.9, CompactUpgrades.h). Room pixel coordinates.

namespace {
using UpgradeReal=downwell::MotionBinary64;
// hitIgnoreCheck keys: the target family in the top nibble.
enum : std::uint32_t {
    blastHitEnemy=1u<<28,blastHitBox=2u<<28,blastHitScattered=3u<<28,blastHitFire=4u<<28,
    blastHitModule=5u<<28,blastHitGemChunk=6u<<28,blastHitCorpse=7u<<28
};
}

void GameplayWorld::clearCompactUpgrades(){
    for(unsigned i=0;i<blastEnd_;++i)blasts_[i]={};
    blastEnd_=0;
    drone_={};firedThisStep_=false;balloon_={};
    if(jetSound_>=0){run_.stopSound(jetSound_);jetSound_=-1;}
    for(unsigned i=0;i<bloodEnd_;++i)bloods_[i]={};
    bloodEnd_=0;
}

// scrPlayerInit's companions at the new player's x/y: pugDecoy's
// decoyBalloon, then pugDrone's objDrone (no Create RNG).
bool GameplayWorld::createCompactCompanions(int x,int y){
    drone_={};balloon_={};
    if(run_.upgrades.levels[15]){
        balloon_.live=true;balloon_.x=x;balloon_.y=y;
        balloon_.xx=x*65536;balloon_.yy=y*65536;
    }
    if(run_.upgrades.levels[9]){
        drone_.live=true;drone_.x=long(x)<<16;drone_.y=long(y)<<16;
    }
    return true;
}

// instance_create(x, y, bulletDrone); bDir and bdmg are the creator's.
GameplayWorld::BulletState* GameplayWorld::createDroneBullet(int x,int y,int degrees,int damage){
    BulletState* bullet=birthBullet(x-240,y-176,459);
    if(!bullet)return nullptr;
    bullet->kind=BulletState::ranged;bullet->damage=damage;
    bullet->shot.createDrone(x-240,y-176,degrees);
    return bullet;
}

// objDrone Alarm 0: scrEffectSpawn(x, y + 12, 603, 0.5, 270, -60000), the
// shot at (x, y + 8) towards 270, soundPlay(47, 60), droneShooting = 0.
bool GameplayWorld::advanceCompactDroneAlarm(){
    if(!drone_.live||drone_.alarm0<0||--drone_.alarm0>0)return true;
    drone_.alarm0=-1;
    const int x=int((drone_.x+0x8000)>>16),y=int((drone_.y+0x8000)>>16);
    if(!compactEffects_.spawnNoLoop(downwell::Fixed16::fromInteger(x),downwell::Fixed16::fromInteger(y+12),603,half,
        downwell::Fixed16::fromInteger(270)))return run_.effectsSucceeded=false;
    if(!createDroneBullet(x,y+8,270,4))return false;
    run_.soundPlay(47,60,false,true);
    drone_.shooting=false;
    return run_.effectsSucceeded;
}

// objDrone Step: ease towards (plx + lengthdir_x(24, xdir), ply +
// lengthdir_y(8, ydir) - 24) by a tenth; a shot this update arms Alarm 0.
void GameplayWorld::stepCompactDrone(){
    if(!drone_.live)return;
    drone_.xdir=(drone_.xdir+2)%360;drone_.ydir=(drone_.ydir+2)%360;
    const long px=long(240+int(downwell::roundGameMakerQ8(player_.offsetXFixed())))<<16;
    const long py=long(176+int(downwell::roundGameMakerQ8(player_.offsetYFixed())))<<16;
    // 24 cos and -8 sin in Q16 from the Q30 sine magnitudes; the sign is
    // the phase's half turn.
    const auto signedSine=[](std::uint32_t phase){
        const auto magnitude=std::int32_t(downwell::LevelClearTrig::sineQ30(phase));
        return (phase&0x80000000u)?-magnitude:magnitude;
    };
    const std::int32_t cosine=signedSine(downwell::RangedShot::degreesToTurns(drone_.xdir)+0x40000000u);
    const std::int32_t sine=signedSine(downwell::RangedShot::degreesToTurns(drone_.ydir));
    const long xl=long(downwell::LevelClearTrig::shift(cosine,14))*24,yl=-long(downwell::LevelClearTrig::shift(sine,14))*8;
    drone_.x+=((px-drone_.x)+xl)/10;
    drone_.y+=((py-drone_.y)+yl-(24L<<16))/10;
    if(firedThisStep_&&!drone_.shooting){drone_.shooting=true;drone_.alarm0=4;}
}

// bulletBlast 276 Create (bdmg 50, bDir 270, scrSShake(6, 8), momentDelay,
// image_speed 0.5, image_xscale = choose(1, -1)); bulletExplosion1 272 and
// its child explosionSmall 274 (sprite 470) add sfxEnmExplode on "gunwall",
// imgSp = random_range(0.25, 0.4), image_angle = choose(0, 90, 180, 270) and
// step off other explosions by choose(-3, 3). A nonzero sprite is the
// creator's override after Create (Rocket Jump's spSplosion1).
bool GameplayWorld::createCompactBlast(int object,int x,int y,int sprite){
    if(!run_.effectsSucceeded)return false;
    unsigned slot=0;
    while(slot<blastEnd_&&blasts_[slot].live)++slot;
    if(slot==downwell::gameplayCompactBlastCapacity)return run_.effectsSucceeded=false;
    if(slot==blastEnd_)++blastEnd_;
    auto& b=blasts_[slot];
    b={};b.live=true;b.object=object;b.x=x;b.y=y;b.serial=++blastSerial_;
    b.sprite=object==276?471:object==272?463:470;
    if(!requestCameraShake(6,8))return false;
    if(object!=276)run_.soundPlayOL(91+int(enemyRandom_.chooseIndexSmall(3)),60,false,true,"gunwall");
    compactMomentDelay();
    if(object==276){
        b.imageSpeed={0x3f000000u};
    }else{
        b.speed=downwell::CompactBlastRecord::explosionSpeed(enemyRandom_.nextU32());
        b.imageSpeed=b.speed;
    }
    b.xscale=enemyRandom_.chooseIndexSmall(2)?-1:1;
    if(object!=276){
        b.turn=int(enemyRandom_.chooseIndexSmall(4));
        // collision_point(x, y, bulletExplosion1, precise, notme).
        for(unsigned guard=0;;++guard){
            bool met=false;
            for(unsigned i=0;i<blastEnd_&&!met;++i){
                const auto& other=blasts_[i];
                if(i==slot||!other.live||other.object==276)continue;
                const auto r=other.bounds();
                met=b.x>=r.left&&b.x<r.right&&b.y>=r.top&&b.y<r.bottom;
            }
            if(!met)break;
            if(guard==4096)return run_.effectsSucceeded=false;
            b.x+=enemyRandom_.chooseIndexSmall(2)?3:-3;
            b.y+=enemyRandom_.chooseIndexSmall(2)?3:-3;
        }
    }
    if(sprite)b.sprite=sprite;
    return true;
}

// scrEnemyStomped's and the scenery stomps' pugExplodingKnees block: the
// blast at the stomped instance, soundPlay(choose(91, 92, 93), 60) and, for
// enemies, emitSmoke at (plx, ply + 4) towards 0 and 180 (4) and 15 and 165 (3).
bool GameplayWorld::compactExplodingKnees(int x,int y,int smokeX,int smokeY,bool smoke){
    if(!createCompactBlast(276,x,y))return false;
    run_.soundPlay(91+int(enemyRandom_.chooseIndexSmall(3)),60,false,true);
    if(!smoke||!compactDamageEffectsAdmitted)return run_.effectsSucceeded;
    static const int puffs[4][2]={{0,4},{180,4},{15,3},{165,3}};
    for(const auto& puff:puffs)
        if(!run_.smokeEffects.emitDirected(downwell::Fixed16::fromInteger(smokeX),downwell::Fixed16::fromInteger(smokeY+4),
            downwell::Fixed16::fromInteger(puff[0]),downwell::Fixed16::fromInteger(puff[1]),enemyRandom_))
            return run_.effectsSucceeded=false;
    return true;
}

// scrJump's pugRocket block: the blast at (x, y + 4) as spSplosion1,
// soundPlay(344, 60) and emitSmoke(plx, ply + 4) towards 15 and 165 (3).
bool GameplayWorld::compactRocketJump(int x,int y,int smokeX,int smokeY){
    if(!createCompactBlast(276,x,y+4,461))return false;
    run_.soundPlay(344,60,false,true);
    if(!compactDamageEffectsAdmitted)return run_.effectsSucceeded;
    static const int directions[2]={15,165};
    for(int direction:directions)
        if(!run_.smokeEffects.emitDirected(downwell::Fixed16::fromInteger(smokeX),downwell::Fixed16::fromInteger(smokeY+4),
            downwell::Fixed16::fromInteger(direction),downwell::Fixed16::fromInteger(3),enemyRandom_))
            return run_.effectsSucceeded=false;
    return true;
}

// Animation precedes Begin Step; Animation End destroys all three objects.
void GameplayWorld::advanceCompactBlastAnimation(){
    for(unsigned i=0;i<blastEnd_;++i){
        auto& b=blasts_[i];
        if(b.live&&!b.advanceAnimation()){run_.effectsSucceeded=false;return;}
    }
    while(blastEnd_&&!blasts_[blastEnd_-1].live)--blastEnd_;
}

// Step: the damage frames, and bulletExplosion1's TimeStopBound speed.
bool GameplayWorld::stepCompactBlasts(std::uint32_t lastSerial){
    for(unsigned i=0;i<blastEnd_;++i){
        auto& b=blasts_[i];
        if(!b.live||b.serial>lastSerial)continue;
        b.step(b.object==276||timeStopBound(b.bounds(),b.x));
    }
    return true;
}

// scrTypicalExplosionDamage (bConsistent: hitIgnoreCheck, never destroyed):
// hit, hitDir = bDir (270), scrDamageEnemy, soundPlay(29, 90), scrBulletImpact
// and scrEffectSpawn(x, y, 605, 0.4, bDir, 0), momentDelay.
bool GameplayWorld::blastEnemy(downwell::CompactBlastRecord& blast,EnemyState& enemy){
    if(enemy.objectId==112||enemy.objectId==114||enemy.health<=0)return true; // bulletImmune; ehp > 0
    if(!blast.remember(blastHitEnemy|(enemy.lifetimeId&0x0fffffffu)))return true;
    const std::uint32_t down=0xc0000000u; // 270 degrees
    const int damage=50;
    if(enemy.objectId==115||enemy.objectId==118||enemy.objectId==173){
        if(!hitSteeringEnemy(enemy,down,damage))return false;
        run_.soundPlay(29,90,false,true);
    }else{
        // As for bullets, sound 29 precedes the death the compact world runs
        // inside the damage.
        run_.soundPlay(29,90,false,true);
        enemy.takenImpactDamage=damage;
        const auto impact=downwell::applyBulletDamage(enemy,run_.kills,enemyRandom_,
            [this,&enemy]{run_.combo.state.enemyDefeated();return compactEnemyDeath(enemy,false);},
            damage,down,[this](const EnemyState& record){return publishEnemyActivation(record,false);});
        if(impact.kind==downwell::BulletImpactKind::failed)return run_.effectsSucceeded=false;
        compactMomentDelay(); // scrDamageEnemy
    }
    if(compactDamageEffectsAdmitted&&!compactEffects_.spawnNoLoop(downwell::Fixed16::fromInteger(blast.x),
        downwell::Fixed16::fromInteger(blast.y),605,fourTenths,downwell::Fixed16::fromInteger(270)))
        return run_.effectsSucceeded=false;
    compactMomentDelay();
    return run_.effectsSucceeded;
}

// parentShootableThing: objHp -= bdmg and scrFlashballfx(x, y, 1, 0, 0).
bool GameplayWorld::blastShootables(downwell::CompactBlastRecord& blast,const downwell::ContactBounds& bounds){
    const auto hit=[&](std::uint32_t key,int& objHp){
        if(!blast.remember(key)||objHp<=0)return true;
        objHp-=50;
        return run_.emitFlash<1,0,0>(downwell::Fixed16::fromInteger(blast.x),downwell::Fixed16::fromInteger(blast.y),enemyRandom_);
    };
    for(unsigned i=0;i<guns_.moduleEnd;++i){auto& m=guns_.modules[i];
        if(m.live&&downwell::contactBoundsOverlap(bounds,downwell::contactPixelBounds(m.x,m.y,1,1,m.sprite()))&&
           !hit(blastHitModule|i,m.objHp))return false;
    }
    auto& sc=scenery_;
    for(unsigned i=0;i<sc.scatteredEnd;++i){auto& r=sc.scattered[i];
        if(r.live&&downwell::contactBoundsOverlap(bounds,downwell::contactPixelBounds(r.x,r.y,r.scaleX,1,550))&&
           !hit(blastHitScattered|i,r.objHp))return false;
    }
    for(unsigned i=0;i<sc.fireEnd;++i){auto& r=sc.fires[i];
        if(r.live&&r.masked&&downwell::contactBoundsOverlap(bounds,downwell::contactPixelBounds(r.x,r.y,1,1,710))&&
           !hit(blastHitFire|i,r.objHp))return false;
    }
    // Rest in Pieces' corpses (objHp 1 once active): one blast sets off the
    // next, as the source's chain does.
    if(!run_.upgrades.levels[3]||!compactCorpses_.size())return run_.effectsSucceeded;
    struct Hooks{} hooks;
    if(!compactCorpses_.dispatch(compactCorpses_.beginPhase(),[&](std::uint32_t handle,downwell::CorpseAnimation& corpse){
        std::int32_t cx,cy;
        if(!corpse.x.roundPixel(cx)||!corpse.y.roundPixel(cy))return false;
        if(!downwell::contactBoundsOverlap(bounds,downwell::contactPixelBounds(cx,cy,1,1,599))||
           !blast.remember(blastHitCorpse|(handle&0x0fffffffu)))return true;
        int order;
        if(!downwell::compareRunnerReal(corpse.health,{},order))return false;
        if(order<=0)return true;
        return corpse.health.add(downwell::MotionBinary64::fromInteger(-50))&&
            run_.emitFlash<1,0,0>(downwell::Fixed16::fromInteger(blast.x),downwell::Fixed16::fromInteger(blast.y),enemyRandom_);
    },hooks))return run_.effectsSucceeded=false;
    return run_.effectsSucceeded;
}

// subparentShootableWall (objBox_n, gemChunk): wallHp -= bdmg while positive.
bool GameplayWorld::blastWalls(downwell::CompactBlastRecord& blast,const downwell::ContactBounds& bounds){
    for(unsigned i=0;i<breakableCount_;++i){
        auto& box=breakables_[i];
        if(!box.active||box.destroyRequested||!downwell::contactBoundsOverlap(bounds,
            {box.x+240-8,box.x+240+8,box.y+176-8,box.y+176+8}))continue;
        if(!blast.remember(blastHitBox|(box.lifetimeId&0x0fffffffu))||box.state.exhausted())continue;
        auto damage=UpgradeReal::fromInteger(-50);
        if(!box.state.wallHp.add(damage))return run_.effectsSucceeded=false;
        observeBoxHealth(box);
    }
    for(unsigned i=0;i<scenery_.gemChunkEnd;++i){
        auto& r=scenery_.gemChunks[i];
        if(!r.live||!downwell::contactBoundsOverlap(bounds,downwell::contactPixelBounds(r.x,r.y,1,1,569)))continue;
        if(blast.remember(blastHitGemChunk|i)&&r.wallHp>0)r.wallHp-=50;
    }
    return run_.effectsSucceeded;
}

// Collision events while damaging, in each object's event order:
// bulletBlast parentShootableThing, parentWall (parent 84), subparentEnemy;
// bulletExplosion1 parentShootableThing, subparentShootableWall, subparentEnemy.
bool GameplayWorld::collideCompactBlasts(){
    for(unsigned i=0;i<blastEnd_;++i){
        auto& b=blasts_[i];
        if(!b.live||!b.damaging())continue;
        const auto bounds=b.bounds();
        if(!blastShootables(b,bounds)||!blastWalls(b,bounds))return false;
        for(unsigned e=0;e<enemyCount_;++e){
            auto& enemy=enemies_[e];
            if(!enemy.active||enemy.destroyRequested||enemy.health<=0)continue;
            const auto target=downwell::contactWorldBounds({(enemy.x+240L)*256,(enemy.y+176L)*256,
                enemy.objectId==117?enemy.motionState:enemy.facing,enemy.imageScaleY(),
                downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)});
            if(downwell::contactBoundsOverlap(bounds,target)&&!blastEnemy(b,enemy))return false;
        }
    }
    return run_.effectsSucceeded;
}

// ---------------------------------------------------------------------------
// Knife and Fork (upgrade 11, global.pugKnifefork).

// fxBlood Create at the player; a full store drops the cosmetic drop.
bool GameplayWorld::createCompactBlood(int x,int y){
    unsigned slot=0;
    while(slot<bloodEnd_&&bloods_[slot].live)++slot;
    if(slot==downwell::gameplayCompactBloodCapacity)return true;
    if(slot==bloodEnd_)++bloodEnd_;
    auto& b=bloods_[slot];
    b={};b.live=true;b.birth=tick_;
    return b.create(x,y,enemyRandom_)||(run_.effectsSucceeded=false);
}

// objDeadBody's objPlayer_n collision: an active body is eaten (sfx 83 on
// the UI group, five fxBlood at plx/ply) and destroyed. plx/ply are whole
// pixels, re-read per child but unchanged by the Creates.
bool GameplayWorld::eatCompactCorpses(){
    if(!run_.upgrades.levels[11]||!compactCorpses_.size())return true;
    const auto player=downwell::contactWorldBounds({240L*256+player_.offsetXFixed(),176L*256+player_.offsetYFixed(),1,1,31});
    const int px=240+int(downwell::roundGameMakerQ8(player_.offsetXFixed()));
    const int py=176+int(downwell::roundGameMakerQ8(player_.offsetYFixed()));
    struct Hooks{} hooks;
    const bool playerDead=dead();
    return compactCorpses_.dispatch(compactCorpses_.beginPhase(),[&](auto,downwell::CorpseAnimation& corpse){
        std::int32_t cx,cy;
        if(!corpse.x.roundPixel(cx)||!corpse.y.roundPixel(cy))return false;
        if(!downwell::contactBoundsOverlap(player,downwell::contactPixelBounds(cx,cy,1,1,599)))return true;
        if(!corpse.valid)return false;
        if(corpse.destroyRequested||playerDead||!corpse.active)return true;
        ++run_.bodiesEaten;
        run_.soundPlayOL(83,80,false,true,"UI");
        for(unsigned i=0;i<5;++i)if(!createCompactBlood(px,py))return corpse.valid=false;
        corpse.destroyRequested=true;
        return run_.effectsSucceeded;
    },hooks)||(run_.effectsSucceeded=false);
}

bool GameplayWorld::advanceCompactBloodAnimation(){
    for(unsigned i=0;i<bloodEnd_;++i)
        if(bloods_[i].live&&!bloods_[i].advanceImage())return run_.effectsSucceeded=false;
    return true;
}

// fxBlood Step (gravity 0.2, global.fallspeed 0) and Alarm 0 (120).
bool GameplayWorld::stepCompactBlood(){
    for(unsigned i=0;i<bloodEnd_;++i){auto& b=bloods_[i];
        if(!b.live)continue;
        if(b.alarm0>0&&--b.alarm0==0){b.live=false;continue;}
        if(!b.step())return run_.effectsSucceeded=false;
    }
    while(bloodEnd_&&!bloods_[bloodEnd_-1].live)--bloodEnd_;
    return true;
}

// fxBlood's parentWall collision destroys it (sampled at its pixel).
bool GameplayWorld::collideCompactBlood(){
    for(unsigned i=0;i<bloodEnd_;++i){auto& b=bloods_[i];
        if(!b.live)continue;
        const int px=b.pixelX(),py=b.pixelY();
        if(solidResolver_&&solidResolver_(solidResolverContext_,px-240,py-176))b.live=false;
    }
    return true;
}

// scrControlBeginstepCheck (objControlerN Begin Step): every ten bodies
// eaten give one heart and a rising text at the player. Its scrFxNol 108
// is created at the persistent controller's rmInit position (-64, 80),
// outside every room view, and is omitted.
bool GameplayWorld::checkKnifeAndFork(){
    if(!run_.upgrades.levels[11]||run_.bodiesEaten<10)return true;
    (void)run_.combo.healthReserve.gain(health_,1);
    run_.bodiesEaten-=10;
    if(auto* n=downwell::CompactGuns::append(guns_.notices,guns_.noticeEnd)){
        *n={};n->live=true;n->fixedText=downwell::CompactItemNoticeRecord::omnom;
        n->x=240+int(downwell::roundGameMakerQ8(player_.offsetXFixed()));
        n->y=long(176+int(downwell::roundGameMakerQ8(player_.offsetYFixed())))<<16;n->birth=tick_;
    }
    return true;
}

// collision_point(x, y, parentWall) at a room pixel: terrain, or near a shop
// its walls and table.
bool GameplayWorld::compactSolidPoint(int x,int y)const{
    return (solidResolver_&&solidResolver_(solidResolverContext_,x-240,y-176))||
        (run_.shops.allocatedCount&&run_.shops.wallMaterialAt(x,y,false)>=0);
}

// decoyBalloon Step. Held: the string's top point climbs from the player
// (global.plx/ply, this Step's scrPlMovement entry) until a wall is 4 pixels
// above it or it is stringLength up (10 while spin-jumping); the balloon
// eases there, and its art follows the horizontal pull. Loose: it is picked
// up when the player crosses its string, and otherwise rises to 0.8.
bool GameplayWorld::stepCompactBalloon(){
    auto& b=balloon_;
    const auto fail=[&]{return run_.effectsSucceeded=false;};
    if(b.held){
        b.stringLength=player_.spinJumping()?10:20;
        // plx/ply in Q16: the frame-start Q8 position plus the room offset.
        const long fx=player_.frameStartXFixed(),fy=player_.frameStartYFixed();
        if(fx< -0x200000L||fx>0x200000L||fy< -0x200000L||fy>0x200000L)return fail();
        // pointy is ply minus whole pixels; Q8 floors give the tested pixel.
        const int px=240+int(fx>>8),py=176+int(fy>>8);
        int climb=0;
        while(!compactSolidPoint(px,py-climb-4))if(++climb>b.stringLength)break;
        std::int32_t xMove;
        if(!b.ease(std::int32_t(fx)*256+240*65536,std::int32_t(fy)*256+176*65536,climb,xMove)||!b.pose(xMove))
            return fail();
    }
    if(!b.held){
        // collision_line(x, y, x, y + stringLength + 4, objPlayer_n); noHold
        // stays 0 in world one.
        const auto player=downwell::contactWorldBounds({240L*256+player_.offsetXFixed(),
            176L*256+player_.offsetYFixed(),1,1,31});
        if(b.x>=player.left&&b.x<player.right&&b.y<player.bottom&&b.y+b.stringLength+4>=player.top)b.held=true;
        bool hit=false;
        if(!solidMeetingQ16(526,b.xx,b.yy+b.ysp,false,hit)||!b.rise(hit))return fail();
    }
    b.publish();return true;
}

bool GameplayWorld::advanceCompactBalloonAnimation(){
    bool crossed=false;
    return downwell::UnitSequenceFrames::advance(balloon_.imageIndex,balloon_.imageSpeed,4,balloon_.imageIndex,crossed)||
        (run_.effectsSucceeded=false);
}

// decoyBalloon Collision with subparentEnemy: three bulletExplosion1 and three
// explosionSmall at the balloon, then it is gone and global.ballooning is 0.
bool GameplayWorld::collideCompactBalloon(){
    const auto balloon=balloon_.bounds();
    for(unsigned e=0;e<enemyCount_;++e){const auto& enemy=enemies_[e];
        if(!enemy.active||enemy.destroyRequested)continue;
        const auto target=downwell::contactWorldBounds({(enemy.x+240L)*256,(enemy.y+176L)*256,
            enemy.objectId==117?enemy.motionState:enemy.facing,enemy.imageScaleY(),
            downwell::enemyContactMask(enemy.objectId,enemy.spriteId,enemy.maskId)});
        if(!downwell::contactBoundsOverlap(balloon,target))continue;
        const int x=balloon_.x,y=balloon_.y;
        balloon_={};
        for(int i=0;i<3;++i)if(!createCompactBlast(272,x,y))return false;
        for(int i=0;i<3;++i)if(!createCompactBlast(274,x,y))return false;
        return true;
    }
    return true;
}
