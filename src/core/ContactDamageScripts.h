#ifndef DOWNWELL_CORE_CONTACT_DAMAGE_SCRIPTS_H
#define DOWNWELL_CORE_CONTACT_DAMAGE_SCRIPTS_H
#include "TypicalPlayerDamage.h"
#include "PlayerDamage.h"
#include "ForcedPlayerJump.h"
#include "MusicDuck.h"
namespace downwell {
// VM self storage, owned by the attacking instance, not the player. Keep it
// through destruction marking and failed synchronous child calls. These are
// distinct source fields: pdmg/p?impact/pdir, hitInvi, and ?fjump/?fjumpdef.
// Object-name/string resolution remains the damage event adapter's job.
struct ContactDamageScripts {
    TypicalPlayerDamage typical;
    PlayerDamage player;
    ForcedPlayerJump jump;
    MusicDuck music;
    template<class Events,class CreateField>
    bool execute(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
        Events& events,CreateField createField){
        return typical.executeCalls(amount,horizontal,vertical,events,createField,
            [&](MotionBinary64 value){return player.execute(value,events);},
            [&](MotionBinary64 x,MotionBinary64 y){return jump.execute(x,y,events);});
    }
};
}
#endif
