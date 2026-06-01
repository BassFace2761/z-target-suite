#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern u32 recomp_get_config_u32(const char* name);

RECOMP_HOOK("Player_Update") void Hook_MuteLockOffSound(Actor* thisx, PlayState* play) {
    Player* player = (Player*)thisx;
    u32 immersive = recomp_get_config_u32("immersive_targeting");

    if (immersive) {
        // Actor_UpdateAll triggers the lock-off sound if reticleSpinCounter != 0
        // when a lock-on is broken (focusActor == NULL or zTargetActiveTimer < 5).
        // By forcing it to 0 right before Actor_UpdateAll evaluates it, we ninja-silence the sound!
        if ((player->focusActor == NULL) || (player->zTargetActiveTimer < 5)) {
            play->actorCtx.attention.reticleSpinCounter = 0;
        }
    }
}