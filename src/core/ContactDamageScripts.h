#ifndef DOWNWELL_CORE_CONTACT_DAMAGE_SCRIPTS_H
#define DOWNWELL_CORE_CONTACT_DAMAGE_SCRIPTS_H
#include "TypicalPlayerDamage.h"
#include "PlayerDamage.h"
#include "ForcedPlayerJump.h"
namespace downwell {
// VM self storage, owned by the attacking instance, not the player. Keep it
// through destruction marking and failed synchronous child calls. These are
// distinct source fields: pdmg/p?impact/pdir, hitInvi, and ?fjump/?fjumpdef.
// Object-name/string resolution remains the damage event adapter's job.
struct ContactDamageScripts {
    TypicalPlayerDamage typical;
    PlayerDamage player;
    ForcedPlayerJump jump;
};
}
#endif
