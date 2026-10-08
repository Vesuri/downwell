#ifndef DOWNWELL_CORE_BUILDER_COMMON_DISPATCH_H
#define DOWNWELL_CORE_BUILDER_COMMON_DISPATCH_H
namespace downwell {
enum class BuilderChildField {deactivate, maskIndex, indexVariable, imageIndex, checked, isPillar};
// scrBuilder VM blocks 119--172 and pillar post-Create blocks 177--186/197--206.
// Ops.create(object,x,y,post) must run the full synchronous Create before post.
// post receives an owner handle, never a slot or a guessed numeric instance ID.
// Ops.write/deactivate/rightOfMiddle act on that same child. rightOfMiddle must
// read the child's published X after Create, using the runner comparison.
struct BuilderCommonDispatch {
    int chunkOrCrate=0, sideroomRightmost=0;
    bool succeeded=true;
    template<class Ops> bool cell(char symbol,int x,int y,int roomWidth,int playerHp,
                                  unsigned style,Ops& ops){
        if(!succeeded)return false;
        auto plain=[&](int object){return ops.create(object,x,y,[](auto){return true;});};
        auto wall=[&](){return ops.create(60,x,y,[&](auto child){
            return ops.write(child,BuilderChildField::deactivate,1);
        });};
        auto box=[&](){return ops.create(92,x,y,[&](auto child){return ops.deactivate(child);});};
        static const int coin[]={0,1}, rare[]={0,0,0,1}, crate[]={254};
        static const int reward[]={293,109}, favorGun[]={0,1,1,1,1,1};
        bool ok=true;
        switch(symbol){
        case '1':ok=wall();break;
        case '2':if(ops.choose(coin,2))ok=wall();break;
        case '3':if(ops.choose(coin,2))ok=box();break;
        case 'B':ok=box();break;
        case '-':ok=plain(93);break;
        case 'W':ok=plain(103);break;
        case 'N':ok=plain(329);break;
        case '$':ok=plain(277);break;
        case '\\':ok=plain(285);break;
        case 'v':ok=plain(376);break;
        case '@':
            if(!chunkOrCrate)chunkOrCrate=ops.choose(reward,2);
            else chunkOrCrate=chunkOrCrate==109?293:109;
            if(playerHp<=2 && chunkOrCrate!=109 && ops.choose(favorGun,6))chunkOrCrate=109;
            if(style==1)chunkOrCrate=109;
            ok=plain(chunkOrCrate);break;
        case '+':ok=plain(109);break;
        case 'O':ok=plain(82);break;
        case '?':if(ops.choose(rare,4))ok=plain(ops.choose(crate,1));break;
        case '=':ok=plain(306);break;
        case '_':ok=plain(62);break;
        case '|':ok=plain(60);break;
        case '{':case '}':
            // Integral buildx/room_width domain: 2*x < roomWidth exactly
            // matches the VM's epsilon comparison even for odd room widths.
            if(roomWidth<0)return succeeded=false;
            sideroomRightmost=static_cast<long long>(x)*2<roomWidth?-16:roomWidth-144;
            ok=ops.create(60,sideroomRightmost,y,[&](auto child){
                return ops.write(child,BuilderChildField::maskIndex,symbol=='{'?594:595)&&
                       ops.write(child,BuilderChildField::indexVariable,15)&&
                       ops.write(child,BuilderChildField::imageIndex,15)&&
                       ops.write(child,BuilderChildField::checked,1);
            });break;
        default:break;
        }
        return succeeded=succeeded&&ok;
    }
    template<class Ops> bool pillar(int x,int y,int roomWidth,Ops& ops){
        if(!succeeded)return false;
        const bool ok=ops.create(60,x,y,[&](auto child){
            bool right=false;
            return ops.rightOfMiddle(child,roomWidth,right)&&
                   ops.write(child,BuilderChildField::isPillar,right?1:-1);
        });
        return succeeded=succeeded&&ok;
    }
};
}
#endif
