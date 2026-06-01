#include "global.h"
#include "modding.h"
#include "recompconfig.h"

RECOMP_PATCH f32 Attention_WeightedDistToPlayerSq(Actor* actor, Player* player, s16 playerShapeYaw) {
    f32 adjDistSq;
    s16 yawDiffAbs = ABS_ALT(BINANG_SUB(BINANG_SUB(actor->yawTowardsPlayer, 0x8000), playerShapeYaw));

    u32 targetBehindEnabled = recomp_get_config_u32("target_behind");

    // 65% of the standard 1000.0f targeting range is 650.0f.
    // We use squared distance here, so 650.0f * 650.0f = 422500.0f
    f32 maxBehindDistSq = 422500.0f;

    if (player->focusActor != NULL) {
        if (actor->flags & ACTOR_FLAG_LOCK_ON_DISABLED) {
            return FLT_MAX;
        }

        if (!targetBehindEnabled && (yawDiffAbs > 0x4000)) {
            return FLT_MAX;
        }

        // MOD: If behind the player, enforce the 65% range limit
        if (targetBehindEnabled && (yawDiffAbs > 0x4000) && (actor->xyzDistToPlayerSq > maxBehindDistSq)) {
            return FLT_MAX;
        }

        // The vanilla math naturally penalizes actors behind the player by inflating their distance weight.
        adjDistSq = actor->xyzDistToPlayerSq - ((actor->xyzDistToPlayerSq * 0.8f) * ((0x4000 - yawDiffAbs) * (1.0f / 0x8000)));

        return adjDistSq;
    }

    // --- When NOT currently targeting something ---

    // Vanilla behavior
    if (!targetBehindEnabled) {
        if (yawDiffAbs > (0x10000 / 6)) {
            return FLT_MAX;
        }
        return actor->xyzDistToPlayerSq;
    }

    // Mod ON behavior
    // Enforce the 65% range limit for off-screen targets
    if ((yawDiffAbs > 0x4000) && (actor->xyzDistToPlayerSq > maxBehindDistSq)) {
        return FLT_MAX;
    }
    
    // Apply priority weighting even when unfocused for a better feeling lock-on
    return actor->xyzDistToPlayerSq - ((actor->xyzDistToPlayerSq * 0.8f) * ((0x4000 - yawDiffAbs) * (1.0f / 0x8000)));
}