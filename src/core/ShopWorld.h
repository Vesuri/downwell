#ifndef DOWNWELL_CORE_SHOP_WORLD_H
#define DOWNWELL_CORE_SHOP_WORLD_H
#include "ShopInteraction.h"
#include "ShopFeedback.h"
#include "ShopKeeper.h"
#include "InteractionPrompt.h"
#include "Input.h"
#include "ProjectileSegment.h"
#include "EnemyContact.h"
#include "ShopChildCreation.h"
#include "MotionBinary64.h"
#include "ShopRisingText.h"
#include "ShopText.h"
#include "ShopDrawState.h"
namespace downwell {
// Bounded world ownership and static child masks, separate from shop rendering.
// Coordinates passed here are integer mask-query positions. Full fractional
// GameMaker collision semantics remain a separate runtime audit.
struct ShopWorld {
    static constexpr unsigned capacity = 8;
    struct Instance {
        std::uint32_t sourceId=0;
        ShopChildCreation children;
        int radio=-1;
        bool shopBgm=false;
        bool shopInView=false,keeperAngry=false;
        int x = 0, y = 0;
        bool allocated = false, wallPresent = false;
        unsigned artRevision = 0;
        unsigned farewellVariant = 0; // scrShopAssignText chooses shopBye0..2 once
        bool present = false, maskEnabled = true, interactOk = true;
        ShopInventory stock;
        ShopInteraction menu;
        ShopFeedback feedback;
        ShopKeeper keeper;
        InteractionPrompt prompt;
        ShopRisingText speech;
        MotionBinary64 messageX{};
        ShopText text;
        ShopDrawState draw;
    };
    Instance instances[capacity];
    unsigned allocatedCount = 0;
    unsigned fixtureRevision = 0;
    bool noControl = false, interactable = false;
    Input previous;
    // One mutable input publication for the whole Shop phase. A nested
    // registered with(Shop) must see consumption by earlier parents, rather
    // than reconstructing dUp from held input. Keep raw history separately:
    // consuming dUp must not manufacture another press on the next update.
    ShopInteraction::Input input;

    void publishInput(const Input& held) {
        input = {held.right && !previous.right, held.left && !previous.left,
                 held.fire && !previous.fire, held.cancel && !previous.cancel};
        previous = held;
    }

    void reset() {
        // Do not materialize a whole eight-shop temporary on the A500's
        // small task stack. Preserve every Instance's nonzero defaults.
        for (auto& shop : instances) shop = {};
        allocatedCount = fixtureRevision = 0;
        noControl = interactable = false;
        previous = {};
        input = {};
    }

    // Stock uses irandom_range; scrShopAssignText then chooses one of three
    // farewell strings with a single-word draw. Keep the callbacks distinct.
    template<class Range, class Choice>
    Instance* create(int x, int y, Range random, Choice chooseFarewell, int health, unsigned area,
                     bool hard = false, unsigned style = 0, bool member = false) {
        for (auto& shop : instances) if (!shop.allocated) {
            shop = {};
            shop.x = x; shop.y = y; shop.present = true;
            shop.allocated = shop.wallPresent = true;
            shop.artRevision = 1;
            ++allocatedCount;
            ++fixtureRevision;
            shop.stock.create(random, health, area, hard, style, member);
            shop.farewellVariant = chooseFarewell();
            return &shop;
        }
        return nullptr; // Caller must handle exhaustion; do not consume RNG.
    }
    // Explicit registered creation. The owner must register the parent and
    // execute inherited setup in begin, then supply audio and synchronous child
    // creation and actual retained-instance reads/writes. No fallback owner.
    // The legacy compact create above remains until each backend provides it.
    template<class Range,class Choice,class Owner>
    Instance* createRegistered(int x,int y,Range random,Choice chooseFarewell,int health,unsigned area,
        bool hard,unsigned style,bool member,Owner& owner,bool& good){
        return createRegisteredBody(x,y,random,health,area,hard,style,member,owner,good,[&](Instance& shop){
            shop.farewellVariant=chooseFarewell();return good;
        });
    }
    template<class Range,class Choice,class Owner,class Lookup>
    Instance* createRegisteredWithText(int x,int y,Range random,Choice chooseFarewell,int health,unsigned area,
        bool hard,unsigned style,bool member,Owner& owner,bool& good,Lookup& lookup){
        return createRegisteredBody(x,y,random,health,area,hard,style,member,owner,good,[&](Instance& shop){
            struct Ops {
                Lookup& language;Choice& choose;bool& good;
                bool lookup(const char* key,const char*& text){return language.lookup(key,text)&&good;}
                bool chooseFarewell(unsigned& out){out=choose();return good;}
            } ops{lookup,chooseFarewell,good};
            return shop.text.assign(shop.stock,shop.farewellVariant,ops);
        });
    }
private:
    template<class Range,class Owner,class AssignText>
    Instance* createRegisteredBody(int x,int y,Range random,int health,unsigned area,
        bool hard,unsigned style,bool member,Owner& owner,bool& good,AssignText assignText){
        if(!good)return nullptr;
        for(auto& shop:instances)if(!shop.allocated){
            shop={};shop.x=x;shop.y=y;shop.present=true;
            shop.allocated=shop.wallPresent=true;shop.artRevision=1;
            ++allocatedCount;++fixtureRevision;
            if(!owner.begin(shop)||!good||!shop.sourceId||shop.sourceId>0x7fffffffu){good=false;return nullptr;}
            if(area!=5&&(!owner.play(area==3?201:192,100,true,shop.radio)||!good||
                !owner.setGain(shop.radio,MotionBinary64{},0)||!good)){
                good=false;return nullptr;
            }
            auto create=[&](int object,int cx,int cy)->std::uint32_t{
                if(!good)return 0;const auto id=owner.create(object,cx,cy);return good?id:0;
            };
            if(!shop.children.beforeStock(x,y,create,[&](unsigned id,int& cx){return owner.readX(id,cx)&&good;})){good=false;return nullptr;}
            if(!owner.readXReal(shop.children.keeper,shop.messageX)||!good||
               !shop.messageX.add(MotionBinary64::fromInteger(2))){good=false;return nullptr;}
            shop.stock.create(random,health,area,hard,style,member);
            if(!shop.children.afterStock(shop.stock,create,
                [&](unsigned id,int& cx,int& cy){return owner.position(id,cx,cy)&&good;},
                [&](unsigned id,unsigned value){return owner.upgrade(id,value)&&good;},
                [&](unsigned id,unsigned value){return owner.stored(id,value)&&good;})){
                good=false;return nullptr;
            }
            if(!assignText(shop)||!good){good=false;return nullptr;}return &shop;
        }
        good=false;return nullptr;
    }
public:
    static bool meets(const Instance& shop, int x, int y) {
        // sprShopCounter (480), origin 12,12, margins 2..11 / 11..20;
        // sprPlayerIdle mask -4..3 / -4..7.
        return shop.present && shop.maskEnabled &&
            x + 3 >= shop.x - 10 && x - 4 <= shop.x - 1 &&
            y + 7 >= shop.y - 1 && y - 4 <= shop.y + 8;
    }
    struct Rectangle { int left, top, right, bottom; };
    template<class Emit>
    bool visitWallContacts(const ContactBounds& query,Emit emit)const {
        bool okay=true;
        visitPlayerSolids([&](const Rectangle& wall) {
            const ContactBounds bounds{wall.left,wall.right+1,wall.top,wall.bottom+1};
            if(okay && contactBoundsOverlap(query,bounds))okay=emit();
        });
        return okay;
    }
    template<class Visit>
    void visitPlayerSolids(Visit visit) const {
        if (!allocatedCount) return; // Ordinary first-level queries stay cheap.
        for (const auto& shop : instances) if (shop.allocated) {
            // Shop Create children, not the non-solid counter sprite:
            // table (278/481): origin 32,8; margins 2..61,8..15.
            visit(Rectangle{shop.x + 34, shop.y, shop.x + 93, shop.y + 7});
            // cases (279/482): origin 12,12; margins 3..20,3..20.
            // Buying removes contents only, so sold cases remain solid.
            for (int i = 0; i < 3; ++i) {
                const int caseX = shop.x + 45 + i * 19;
                visit(Rectangle{caseX - 9, shop.y - 18, caseX + 8, shop.y - 1});
            }
            // Invisible wall (98/46), at counter y+8, full 16x16 mask.
            if (shop.wallPresent)
                visit(Rectangle{shop.x - 8, shop.y, shop.x + 7, shop.y + 15});
        }
    }
    bool playerSolidAt(int x, int y) const {
        bool hit = false;
        visitPlayerSolids([&](Rectangle r) {
            if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) hit = true;
        });
        return hit;
    }
    // Current shop children all belong to parentWall (57). The counter and
    // friendly keeper do not. Pure query in room coordinates, suitable for
    // each <=8-pixel LaserSightRay step; never dispatch damage or consume RNG.
    // Do not remove a case merely because wallHp reaches zero: the pinned
    // source has no natural writer that raises its destroyed flag. Registered
    // ownership retains the dormant opening branch and its noMask setter.
    bool rayWallIntersects(const ProjectileSegment& path) const {
        bool hit = false;
        visitPlayerSolids([&](Rectangle r) {
            if (!hit && path.intersects(r.left, r.right + 1, r.top, r.bottom + 1))
                hit = true;
        });
        return hit;
    }
    int playerLanding(int x, int minimumY, int nearest) const {
        visitPlayerSolids([&](Rectangle r) {
            const int landing = r.top - 8;
            if (landing >= minimumY && landing < nearest && x + 3 >= r.left && x - 4 <= r.right)
                nearest = landing;
        });
        return nearest;
    }
    void observePlayer(int x, int y, bool grounded, long speedX) {
        interactable = false;
        if (!grounded || speedX) return;
        for (const auto& shop : instances) if (meets(shop, x, y)) {
            interactable = shop.interactOk;
            return;
        }
    }
    template<class Range>
    void keeperAlarms(Range random) {
        if (!allocatedCount) return;
        for (auto& shop : instances) if (shop.allocated && !shop.sourceId)
            shop.keeper.alarms(shop.feedback, random);
    }
    template<class Range>
    void advanceKeepers(int playerX, bool criminal, unsigned area, Range random) {
        if (!allocatedCount) return;
        for (auto& shop : instances) if (shop.allocated && !shop.sourceId) {
            shop.keeper.step(shop.feedback, playerX, shop.x + 18, criminal, area, random);
            shop.feedback.resetKeeperImage = false; // consumed by the actual child state
            shop.keeper.advanceAnimation();
        }
    }
    // Compact compatibility path only. Registered Draw owns these mutations
    // at its layer slot; a player update must not advance them a second time.
    void advancePresentation() {
        if (!allocatedCount) return;
        for (auto& shop : instances) if (shop.present && !shop.sourceId)
            shop.menu.advancePresentation();
    }
    template<class Choose, class Grant, class Emit>
    void step(const Input& held, int x, int y, bool criminal, bool dead,
              int& facing, CurrencyAccount& currency, Choose choose,
              Grant grant, Emit emit) {
        publishInput(held); // Always track raw edges, including while locked.
        for (auto& shop : instances) if (shop.allocated && !shop.sourceId) {
            shop.feedback.beginStep();
            if (!shop.present) continue;
            const ShopInteraction::Context context{meets(shop, x, y),
                interactable, x > shop.x, criminal, dead};
            shop.menu.step(input, context, noControl, facing, shop.stock,
                currency, choose, grant, [&](ShopInteraction::Effect effect) {
                    if (effect == ShopInteraction::Effect::disableMask)
                        shop.maskEnabled = false;
                    const unsigned cursor = shop.menu.cursor;
                    const bool hadItem = cursor >= 1 && cursor <= 3 && shop.feedback.itemPresent[cursor - 1];
                    shop.feedback.apply(effect, cursor, shop.stock);
                    if (hadItem && !shop.feedback.itemPresent[cursor - 1]) ++shop.artRevision;
                    emit(shop, effect);
                });
            // Tail call in Shop Step: runs even while speaking or dead. Draw
            // suppresses the sign separately when speaking or criminal.
            shop.prompt.step(meets(shop, x, y) && interactable);
        }
    }
    void endStep(int viewY) {
        for (auto& shop : instances) {
            // Registered lifetimes retire through their source End Step and
            // room owner, including audio and deferred destruction publication.
            if(shop.sourceId)continue;
            if (shop.present && (shop.y < viewY - 320 || shop.y > viewY + 3000)) {
                shop.present = false;
                ++fixtureRevision;
                ++shop.artRevision;
            }
            if (shop.wallPresent && (shop.y + 8 < viewY - 160 || shop.y + 8 > viewY + 3000))
                shop.wallPresent = false;
        }
        // Table/cases have no inherited retirement Step; keep them until room
        // teardown even when the counter retires. Do not recycle their owner.
        // Source destruction does not independently release global.noControl.
    }
};
}
#endif
