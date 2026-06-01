#include "global.h"
#include "modding.h"
#include "recompconfig.h"

// 1. The CORRECT struct order straight from z_actor.c!
typedef struct {
    PlayState* play;
    Actor* actor;
    u32 freezeExceptionFlag;
    u32 canFreezeCategory;
    Actor* talkActor;
    Player* player;
    u32 updateActorFlagsMask;
} UpdateActor_Params;

// 2. Declare the internal functions
extern void Actor_SpawnSetupActors(PlayState* play, ActorContext* actorCtx);
extern Actor* Actor_UpdateActor(UpdateActor_Params* params);
extern Actor* Actor_RemoveFromCategory(PlayState* play, ActorContext* actorCtx, Actor* actorToRemove);
extern void Actor_AddToCategory(ActorContext* actorCtx, Actor* actorToAdd, u8 actorCategory);
extern void Attention_Update(Attention* attention, Player* player, Actor* playerFocusActor, PlayState* play);
extern void TitleCard_Update(GameState* gameState, TitleCardContext* titleCtx);
extern void Actor_UpdatePlayerImpact(PlayState* play);
extern void Player_ReleaseLockOn(Player* player);
extern void DynaPoly_UpdateContext(PlayState* play, DynaCollisionContext* dyna);
extern void DynaPoly_UpdateBgActorTransforms(PlayState* play, DynaCollisionContext* dyna);

// 3. Define the static array locally so the linker doesn't crash looking for it
static u32 sCategoryFreezeMasks[ACTORCAT_MAX] = {
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_10000000,
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_10000000,
    PLAYER_STATE1_200,
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_400 | PLAYER_STATE1_10000000,
    PLAYER_STATE1_2 | PLAYER_STATE1_DEAD | PLAYER_STATE1_200,
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000,
    PLAYER_STATE1_2 | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_10000000,
    PLAYER_STATE1_2,
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000,
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_400 | PLAYER_STATE1_10000000,
    PLAYER_STATE1_2,
    PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 | PLAYER_STATE1_10000000,
};

RECOMP_PATCH void Actor_UpdateAll(PlayState* play, ActorContext* actorCtx) {
    s32 category;
    Actor* actor;
    Player* player = GET_PLAYER(play);
    u32* categoryFreezeMaskP;
    s32 newCategory;
    Actor* next;
    ActorListEntry* entry;
    UpdateActor_Params params;

    // Grab our config value!
    u32 immersive = recomp_get_config_u32("immersive_targeting");

    params.player = player;
    params.play = play;

    if (play->soaringCsOrSoTCsPlaying) {
        params.updateActorFlagsMask = ACTOR_FLAG_UPDATE_DURING_SOARING_AND_SOT_CS;
    } else {
        params.updateActorFlagsMask = ACTOR_FLAG_UPDATE_DURING_SOARING_AND_SOT_CS | ACTOR_FLAG_INSIDE_CULLING_VOLUME |
                                      ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    }

    Actor_SpawnSetupActors(play, actorCtx);

    if (actorCtx->unk2 != 0) {
        actorCtx->unk2--;
    }

    categoryFreezeMaskP = sCategoryFreezeMasks;

    if (player->stateFlags2 & PLAYER_STATE2_USING_OCARINA) {
        params.freezeExceptionFlag = ACTOR_FLAG_UPDATE_DURING_OCARINA;
    } else {
        params.freezeExceptionFlag = 0;
    }

    if ((player->stateFlags1 & PLAYER_STATE1_TALKING) && ((player->actor.textId & 0xFF00) != 0x1900)) {
        params.talkActor = player->talkActor;
    } else {
        params.talkActor = NULL;
    }

    for (category = 0, entry = actorCtx->actorLists; category < ACTORCAT_MAX;
         entry++, categoryFreezeMaskP++, category++) {
        params.canFreezeCategory = *categoryFreezeMaskP & player->stateFlags1;
        params.actor = entry->first;

        while (params.actor != NULL) {
            params.actor = Actor_UpdateActor(&params);
        }

        if (category == ACTORCAT_BG) {
            DynaPoly_UpdateContext(play, &play->colCtx.dyna);
        }
    }

    // Move actors to a different actorList if it has changed categories.
    for (category = 0, entry = actorCtx->actorLists; category < ACTORCAT_MAX; entry++, category++) {
        if (entry->categoryChanged) {
            actor = entry->first;

            while (actor != NULL) {
                if (actor->category == category) {
                    actor = actor->next;
                    continue;
                }

                next = actor->next;
                newCategory = actor->category;
                actor->category = category;
                Actor_RemoveFromCategory(play, actorCtx, actor);
                Actor_AddToCategory(actorCtx, actor, newCategory);
                actor = next;
            }
            entry->categoryChanged = false;
        }
    }

    actor = player->focusActor;
    if ((actor != NULL) && (actor->update == NULL)) {
        actor = NULL;
        Player_ReleaseLockOn(player);
    }

    if ((actor == NULL) || (player->zTargetActiveTimer < 5)) {
        actor = NULL;
        if (actorCtx->attention.reticleSpinCounter != 0) {
            actorCtx->attention.reticleSpinCounter = 0;
            
            // MOD: Only play the lock-off sound if Immersive Targeting is disabled
            if (!immersive) {
                Audio_PlaySfx(NA_SE_SY_LOCK_OFF);
            }
        }
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_2)) {
        Attention_Update(&actorCtx->attention, player, actor, play);
    }

    TitleCard_Update(&play->state, &actorCtx->titleCtx);
    Actor_UpdatePlayerImpact(play);
    DynaPoly_UpdateBgActorTransforms(play, &play->colCtx.dyna);
}