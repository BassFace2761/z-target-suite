#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern u32 recomp_get_config_u32(const char* name);

extern struct Actor* sNearestAttentionActor;
extern struct Actor* sPrioritizedAttentionActor;
extern struct Actor* sNearestCameraDriftActor;
extern struct Actor* sPrioritizedCameraDriftActor;
extern f32 sNearestAttentionActorDistSq;
extern f32 sBgmEnemyDistSq;
extern f32 sNearestCameraDriftActorDistSq;
extern s32 sHighestAttentionPriority;
extern s32 sHighestCameraDriftPriority;
extern s16 sAttentionPlayerRotY;

extern f32 Attention_WeightedDistToPlayerSq(Actor* actor, Player* player, s16 playerShapeYaw);
extern s32 Attention_ActorOnScreen(PlayState* play, Actor* actor);
extern s32 Attention_ActorIsInRange(Actor* actor, f32 distSq);

RECOMP_PATCH void Attention_FindActorInCategory(PlayState* play, ActorContext* actorCtx, Player* player, ActorType actorCategory) {
    f32 distSq;
    Actor* actor = actorCtx->actorLists[actorCategory].first;
    Actor* playerFocusActor = player->focusActor;
    s32 isNearestAttentionActor;
    s32 isNearestCameraDriftActor;
    
    // Grab our config value!
    u32 targetBehindEnabled = recomp_get_config_u32("target_behind");

    for (; actor != NULL; actor = actor->next) {
        if ((actor->update == NULL) || ((Player*)actor == player)) {
            continue;
        }

        if (!(actor->flags & (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_CAMERA_DRIFT_ENABLED))) {
            continue;
        }

        if ((actorCategory == ACTORCAT_ENEMY) &&
            CHECK_FLAG_ALL(actor->flags, ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE)) {
            if ((actor->xyzDistToPlayerSq < SQ(500.0f)) && (actor->xyzDistToPlayerSq < sBgmEnemyDistSq)) {
                actorCtx->attention.bgmEnemy = actor;
                sBgmEnemyDistSq = actor->xyzDistToPlayerSq;
            }
        }

        if ((actor == playerFocusActor) && !(actor->flags & ACTOR_FLAG_FOCUS_ACTOR_REFINDABLE)) {
            continue;
        }

        // We still calculate the weighted distance to determine priority sorting
        distSq = Attention_WeightedDistToPlayerSq(actor, player, sAttentionPlayerRotY);

        isNearestAttentionActor =
            (actor->flags & ACTOR_FLAG_ATTENTION_ENABLED) && (distSq < sNearestAttentionActorDistSq);

        isNearestCameraDriftActor =
            (actor->flags & ACTOR_FLAG_CAMERA_DRIFT_ENABLED) && (distSq < sNearestCameraDriftActorDistSq);

        if (!isNearestAttentionActor && !isNearestCameraDriftActor) {
            continue;
        }

        s32 isOnScreen = Attention_ActorOnScreen(play, actor);
        s32 isOnScreenOrBypassed = targetBehindEnabled || isOnScreen;

        if (Attention_ActorIsInRange(actor, actor->xyzDistToPlayerSq) && isOnScreenOrBypassed) {
            CollisionPoly* poly;
            s32 bgId;
            Vec3f lineTestResultPos;

            if (isOnScreen) {
                if (BgCheck_CameraLineTest1(&play->colCtx, &player->actor.focus.pos, &actor->focus.pos, &lineTestResultPos,
                                            &poly, true, true, true, true, &bgId)) {
                    if (!SurfaceType_IsIgnoredByProjectiles(&play->colCtx, poly, bgId)) {
                        continue;
                    }
                }
            }

            if (actor->targetPriority != 0) {
                if (isNearestAttentionActor && (actor->targetPriority < sHighestAttentionPriority)) {
                    sPrioritizedAttentionActor = actor;
                    sHighestAttentionPriority = actor->targetPriority;
                }
                if (isNearestCameraDriftActor && (actor->targetPriority < sHighestCameraDriftPriority)) {
                    sPrioritizedCameraDriftActor = actor;
                    sHighestCameraDriftPriority = actor->targetPriority;
                }
            } else {
                if (isNearestAttentionActor) {
                    sNearestAttentionActor = actor;
                    sNearestAttentionActorDistSq = distSq;
                }
                if (isNearestCameraDriftActor) {
                    sNearestCameraDriftActor = actor;
                    sNearestCameraDriftActorDistSq = distSq;
                }
            }
        }
    }
}