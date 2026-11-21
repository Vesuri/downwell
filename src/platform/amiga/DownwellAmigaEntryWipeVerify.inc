// DOWNWELL_ENTRY_WIPE_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 13
    {
        // Explicit-ID lifecycle fixture, not automatic production admission.
        auto& entries=collisionOwner.entryDithers();downwell::InstanceHandle key;
        auto camera=[](downwell::MotionBinary64& v){v=downwell::MotionBinary64::fromInteger(240);return true;};
        if(!entries.create(400001,camera,[&](downwell::MotionBinary64& v){
            const auto* view=port.roomEntryView();if(!view)return false;v=view->y.promoted();return true;
        },key))return 20;
        for(unsigned frame=1;frame<=120;++frame){
            port.step(0);
            if(!port.effectsSucceeded()||!collisionOwner.valid())return 20;
            const auto* r=entries.find(key);std::int32_t y;
            if(!r||!r->y.promoted().roundPixel(y)||y!=16*int(frame<120?frame:119)||
               r->effect.alarm0!=120-int(frame)||r->destroyRequested!=(frame==120))return 20;
            ++gDownwellEntryLifetimeChecks;
        }
        if(!collisionOwner.prepareDraw()||entries.size()||entries.find(key))return 20;
        // Restore the ordinary initial gameplay state before the independent
        // presentation sweep. The fixture above uses the real room scheduler.
        if(port.restartFromResults(downwell::DeathMenuSequence::Action::Retry,2,initialRingOrigin)!=
            downwell::ResultsDestination::gameplay)return 20;
    }
#endif
#if DOWNWELL_VERIFY_PART == 31
            // Presentation sweep through real registered Draw records. This
            // deliberately supplies fixture positions, not natural Step timing.
            auto& entryFixture=collisionOwner.entryDithers();
            if(!entryFixture.clear()){exitCode=downwellGameplayFailed(__LINE__);break;}
            if(gDownwellEntryWipeFrames<25){
                downwell::InstanceHandle key;
                const int screenY=gDownwellEntryWipeFrames?320+int(gDownwellEntryWipeFrames-1)*16:-116;
                if(!entryFixture.create(400100+gDownwellEntryWipeFrames,
                    [&](downwell::MotionBinary64& value){value=downwell::MotionBinary64::fromInteger(worldViewX+80);return true;},
                    [&](downwell::MotionBinary64& value){value=downwell::MotionBinary64::fromInteger(worldViewY+screenY);return true;},key)){
                    exitCode=downwellGameplayFailed(__LINE__);break;
                }
            }
#endif
#if DOWNWELL_VERIFY_PART == 32
            const bool entryAttached=gDownwellEntryWipeFrames<25;
            const int entryY=gDownwellEntryWipeFrames?320+int(gDownwellEntryWipeFrames-1)*16:-116;
            unsigned entryOffset=0;
            if(actorList.entryCoverVisible!=entryAttached||
                (entryAttached&&(!AmigaEntryMask::offset(entryY,entryOffset)||
                    actorList.entryCover.y!=entryY||actorList.entryCover.offset!=entryOffset))){
                exitCode=downwellGameplayFailed(__LINE__);break;
            }
            const auto& entryList=gameplayScene.copper(back);
            const UWORD* entryWords=entryList.data();
            if(entryWords[entryList.moveOffset(0x100)+1]!=(entryAttached?0x6401:0x4401))++gDownwellEntryWipeFailures;
            for(unsigned reg=0xf0;reg<=0xf4;reg+=4){
                const unsigned at=entryList.moveOffset(reg);
                const ULONG pointer=(ULONG(entryWords[at+1])<<16)|entryWords[at+3];
                const ULONG expected=entryAttached?ULONG(overlayAssetStorage+entryOffset+downwellFetchCropBytes):0;
                if(pointer!=expected)++gDownwellEntryWipeFailures;
            }
            if(!entryAttached){
                if(gameplayScene.coverDetached()!=(gDownwellEntryWipeFrames==26))++gDownwellEntryWipeFailures;
                if(selectOverlayAssetBank(true))++gDownwellEntryWipeFailures; // Not fenced yet.
            }
            for(unsigned i=0;i<AmigaEntryMask::bytes;++i)
                if(overlayAssetStorage[i]!=amiga_entry_mask[i])++gDownwellEntryWipeFailures;
            ++gDownwellEntryWipeFrames;
#endif
#if DOWNWELL_VERIFY_PART == 33
            if(gDownwellEntryWipeFrames==27){
                platform.waitVblank(); // Final detached list is now published.
                if(!gameplayScene.coverDetached()){exitCode=downwellGameplayFailed(__LINE__);break;}
                residentCover=ResidentCover::none;
                if(!selectOverlayAssetBank(true)){exitCode=downwellGameplayFailed(__LINE__);break;}
                for(unsigned i=0;i<gDownwellShopAssetBytes;++i)
                    if(overlayAssetStorage[i]!=amiga_shop_pack[i])++gDownwellEntryWipeFailures;
                ++gDownwellEntryWipeReuses;
                downwellEntryWipeReady();
            }
#endif
