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
    drone_={};firedThisStep_=false;
}

// scrPlayerInit's companions at the new player's x/y: pugDecoy's
// decoyBalloon, then pugDrone's objDrone (no Create RNG).
bool GameplayWorld::createCompactCompanions(int x,int y){
    drone_={};
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
    if(!compactEffects_.spawnNoLoop(UpgradeReal::fromInteger(x),UpgradeReal::fromInteger(y+12),603,{0x3fe00000u,0},
        UpgradeReal::fromInteger(270),UpgradeReal::fromInteger(-60000)))return run_.effectsSucceeded=false;
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
        UpgradeReal speed;
        if(!downwell::randomBinary64(enemyRandom_.nextU32(),{0x3fc33333u,0x33333334u},speed)||
           !speed.add({0x3fd00000u,0})||!b.speed.assign(speed))return run_.effectsSucceeded=false;
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
        if(!run_.smokeEffects.emitDirected(UpgradeReal::fromInteger(smokeX),UpgradeReal::fromInteger(smokeY+4),
            UpgradeReal::fromInteger(puff[0]),UpgradeReal::fromInteger(puff[1]),enemyRandom_))
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
        if(!run_.smokeEffects.emitDirected(UpgradeReal::fromInteger(smokeX),UpgradeReal::fromInteger(smokeY+4),
            UpgradeReal::fromInteger(direction),UpgradeReal::fromInteger(3),enemyRandom_))
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
    if(enemy.objectId==112||enemy.health<=0)return true; // bulletImmune; ehp > 0
    if(!blast.remember(blastHitEnemy|(enemy.lifetimeId&0x0fffffffu)))return true;
    const std::uint32_t down=0xc0000000u; // 270 degrees
    const int damage=50;
    if(enemy.objectId==115||enemy.objectId==118){
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
    if(compactDamageEffectsAdmitted&&!compactEffects_.spawnNoLoop(UpgradeReal::fromInteger(blast.x),
        UpgradeReal::fromInteger(blast.y),605,{0x3fd99999u,0x9999999au},UpgradeReal::fromInteger(270),{}))
        return run_.effectsSucceeded=false;
    compactMomentDelay();
    return run_.effectsSucceeded;
}

// parentShootableThing: objHp -= bdmg and scrFlashballfx(x, y, 1, 0, 0).
bool GameplayWorld::blastShootables(downwell::CompactBlastRecord& blast,const downwell::ContactBounds& bounds){
    const auto hit=[&](std::uint32_t key,int& objHp){
        if(!blast.remember(key)||objHp<=0)return true;
        objHp-=50;
        return run_.emitFlash<1,0,0>(UpgradeReal::fromInteger(blast.x),UpgradeReal::fromInteger(blast.y),enemyRandom_);
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
