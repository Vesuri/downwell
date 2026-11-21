// DOWNWELL_PREPARED_FIELDS_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 1
extern "C" void amigaFieldVerifyCapture(const UBYTE*,const UBYTE*,const UBYTE*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
extern "C" void amigaFieldVerifyMask(const UBYTE*,const UBYTE*,const UBYTE*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
extern "C" void amigaFieldVerifyFinish(const UBYTE*);
extern "C" void amigaFieldVerifyTerrain(const UBYTE*,const UBYTE*,const UBYTE*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
extern "C" int amigaFieldVerifyTerrainFixture(UBYTE*,UBYTE*,UBYTE*);
extern "C" void amigaFieldVerifyCompositeBegin(UBYTE*,UBYTE*);
extern "C" void amigaFieldVerifyCompositeEnd(UBYTE*,UBYTE*);
extern "C" bool amigaFieldVerifyComposite(const UBYTE*,const UBYTE*,const downwell::TimeFieldOutlines&,unsigned);
extern "C" void amigaFieldVerifyColumn(const UBYTE*,const UBYTE*,const downwell::TimeFieldOutlines::Circle&,unsigned,bool);
extern "C" void amigaFieldVerifyCompositeTransfer(const UBYTE*,const UBYTE*,const downwell::TimeFieldOutlines::Circle&,const downwell::TimeFieldOutlines::Circle&,unsigned);
#endif
#if DOWNWELL_VERIFY_PART == 2
    static void validateColumns(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& circle,
        unsigned origin){
        amigaFieldVerifyColumn(terrain,target,circle,origin,true);
        amigaFieldVerifyFinish(target);
    }
    static void validateTerrain(const UBYTE* source,const UBYTE* mask,UBYTE* target,
        unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned w,unsigned h){
        amigaFieldVerifyTerrain(source,mask,target,x,y,sx,sy,w,h);
        // No operation: pixels under the new mask must already be pristine.
        amigaFieldVerifyFinish(target);
    }
#endif
#if DOWNWELL_VERIFY_PART == 8
static bool verifyFieldComposites(UBYTE* terrain,UBYTE* target){
    amigaFieldVerifyCompositeBegin(terrain,target);
    const unsigned mode=gDownwellPreparedFieldMode;
    bool ok=true;
    auto& overlay=gameplayFields[0];
    for(unsigned frame=0;frame<48&&ok;++frame){
        // Captured, pristine, and sparse paths must exchange ownership without
        // retaining pixels from an earlier field. No gameplay RNG is consumed.
        gDownwellPreparedFieldMode=frame%13==0?2:frame%11==0?1:frame%5==0?3:6;
        downwell::TimeFieldOutlines shape;
        shape.count=frame%12==11?0:3;shape.visible=shape.count!=0;
        for(unsigned i=0;i<shape.count;++i){
            const unsigned index=frame&1?2-i:i;
            shape.circles[i]={int(108+(frame%4)*48+index*9),
                int(8+(frame%3)*128+index*5),int(24*256+(frame*37+index*93)%512)};
        }
        if(shape.count&&frame%7==0){
            // Intersect the small rings with the large ring, not merely nest
            // them inside its empty centre. Alternate which one paints last.
            shape.circles[1]={190,140,int(128*256+frame)};
            shape.circles[0]={180,250,int(24*256+frame*3)};
            shape.circles[2]={200,250,int(24*256+frame*5)};
            if(frame&1){const auto first=shape.circles[0];shape.circles[0]=shape.circles[1];shape.circles[1]=first;}
        }
        const unsigned origin=(frame*29)%320;
        ok=overlay.prepare(target,origin,shape,true,[](unsigned){return false;})&&overlay.draw();
        if(ok)ok=amigaFieldVerifyComposite(terrain,target,shape,origin);
    }
    gDownwellPreparedFieldMode=4;
    for(unsigned phase=0;phase<32&&ok;++phase){
        downwell::TimeFieldOutlines shape;
        shape.visible=true;shape.count=1;
        shape.circles[0]={int((phase<16?110:254)+(phase&15)),phase%2?283:0,
            int(128*256+(phase<16?0:511))};
        const unsigned origin=(phase*19)%320;
        ok=overlay.prepare(target,origin,shape,true,[](unsigned){return false;})&&overlay.draw();
        if(ok)ok=amigaFieldVerifyComposite(terrain,target,shape,origin);
    }
    gDownwellPreparedFieldMode=6;
    for(unsigned phase=0;phase<51&&ok;++phase){
        downwell::TimeFieldOutlines shape;shape.visible=true;shape.count=1;
        shape.circles[0]={182+int(phase%17),phase<17?0:phase<34?140:283,
            int(128*256+(phase*73)%512)};
        const unsigned origin=(phase*37)%320;
        ok=overlay.prepare(target,origin,shape,true,[](unsigned){return false;})&&overlay.draw();
        if(ok)ok=amigaFieldVerifyComposite(terrain,target,shape,origin);
    }
    overlay.discard();
    for(unsigned i=0;i<downwellGameRingBytes;++i)target[i]=terrain[i];
    gDownwellPreparedFieldMode=7;
    for(unsigned phase=0;phase<306&&ok;++phase){
        downwell::TimeFieldOutlines shape;shape.visible=true;shape.count=1;
        shape.circles[0]={182+int((phase/6)%17),phase<102?0:phase<204?140:283,
            int(128*256+(phase*19)%512)};
        const unsigned origin=287;
        ok=overlay.prepare(target,origin,shape,true,[](unsigned){return false;},true)&&overlay.draw();
        if(ok)ok=amigaFieldVerifyComposite(terrain,target,shape,origin);
    }
    overlay.discard();gDownwellPreparedFieldMode=mode;
    amigaFieldVerifyCompositeEnd(terrain,target);
    return ok;
}
#endif
