#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern Actor* gCameraDriftActor;
extern void Attention_FindActor(PlayState* play, ActorContext* actorCtx, Actor** actorPtr, Actor** cameraDriftActorPtr, Player* player);
extern void Attention_SetTatlState(Attention* attention, Actor* actor, s32 category, PlayState* play);
extern void Attention_InitReticle(Attention* attention, s32 category, PlayState* play);

RECOMP_PATCH void Attention_Update(Attention* attention, Player* player, Actor* playerFocusActor, PlayState* play) {
    s32 pad;
    Actor* actor; // used for both the Tatl hover actor and reticle actor
    s32 category;
    Vec3f projectedPos;
    f32 invW;

    // Grab our config value!
    u32 immersive = recomp_get_config_u32("immersive_targeting");

    actor = NULL;

    if ((player->focusActor != NULL) &&
        (player->controlStickDirections[player->controlStickDataIndex] == PLAYER_STICK_DIR_BACKWARD)) {
        attention->arrowHoverActor = NULL;
    } else {
        Attention_FindActor(play, &play->actorCtx, &actor, &gCameraDriftActor, player);
        attention->arrowHoverActor = actor;
    }

    if (attention->forcedLockOnActor != NULL) {
        actor = attention->forcedLockOnActor;
        attention->forcedLockOnActor = NULL;
    } else if (playerFocusActor != NULL) {
        actor = playerFocusActor;
    }

    if (actor != NULL) {
        category = actor->category;
    } else {
        category = player->actor.category;
    }

    if ((actor != attention->tatlHoverActor) || (category != attention->tatlHoverActorCategory)) {
        attention->tatlHoverActor = actor;
        attention->tatlHoverActorCategory = category;
        attention->tatlMoveProgressFactor = 1.0f;
    }

    if (actor == NULL) {
        actor = &player->actor;
    }

    if (!Math_StepToF(&attention->tatlMoveProgressFactor, 0.0f, 0.25f)) {
        f32 moveScale = 0.25f / attention->tatlMoveProgressFactor;
        f32 x = actor->focus.pos.x - attention->tatlHoverPos.x;
        f32 y = (actor->focus.pos.y + (actor->lockOnArrowOffset * actor->scale.y)) - attention->tatlHoverPos.y;
        f32 z = actor->focus.pos.z - attention->tatlHoverPos.z;

        attention->tatlHoverPos.x += x * moveScale;
        attention->tatlHoverPos.y += y * moveScale;
        attention->tatlHoverPos.z += z * moveScale;
    } else {
        Attention_SetTatlState(attention, actor, category, play);
    }

    if ((playerFocusActor != NULL) && (attention->reticleSpinCounter == 0)) {
        Actor_GetProjectedPos(play, &playerFocusActor->focus.pos, &projectedPos, &invW);

        if ((projectedPos.z <= 0.0f) || (fabsf(projectedPos.x * invW) >= 1.0f) ||
            (fabsf(projectedPos.y * invW) >= 1.0f)) {
            playerFocusActor = NULL;
        }
    }

    if (playerFocusActor != NULL) {
        if (playerFocusActor != attention->reticleActor) {
            s32 lockOnSfxId;

            Attention_InitReticle(attention, playerFocusActor->category, play);
            attention->reticleActor = playerFocusActor;

            if (playerFocusActor->id == ACTOR_EN_BOOM) {
                attention->reticleFadeAlphaControl = 0;
            }

            lockOnSfxId = CHECK_FLAG_ALL(playerFocusActor->flags, ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE)
                              ? NA_SE_SY_LOCK_ON
                              : NA_SE_SY_LOCK_ON_HUMAN;
            if (!immersive) {
                Audio_PlaySfx(lockOnSfxId);
            }
        }

        attention->reticlePos.x = playerFocusActor->world.pos.x;
        attention->reticlePos.y =
            playerFocusActor->world.pos.y - (playerFocusActor->shape.yOffset * playerFocusActor->scale.y);
        attention->reticlePos.z = playerFocusActor->world.pos.z;

        if (attention->reticleSpinCounter == 0) {
            f32 step = (500.0f - attention->reticleRadius) * 3.0f;
            f32 reticleZoomStep = CLAMP(step, 30.0f, 100.0f);

            if (Math_StepToF(&attention->reticleRadius, 80.0f, reticleZoomStep)) {
                attention->reticleSpinCounter++;
            }
        } else {
            attention->reticleSpinCounter = (attention->reticleSpinCounter + 3) | 0x80;
            attention->reticleRadius = 120.0f;
        }
    } else {
        attention->reticleActor = NULL;
        Math_StepToF(&attention->reticleRadius, 500.0f, 80.0f);
    }
    
    if (immersive) {
        attention->reticleSpinCounter = 0;
    }
}