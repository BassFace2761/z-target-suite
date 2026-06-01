#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern Input* sPlayerControlInput;
extern s32 Player_IsTalking(PlayState* play);
extern void Player_SetParallel(Player* this);

extern void Player_Action_95(Player *this, PlayState *play);

RECOMP_PATCH void Player_UpdateShapeYaw(Player* this, PlayState* play) {
    s16 previousYaw = this->actor.shape.rot.y;

    u32 faster_aim= recomp_get_config_u32("faster_aim_tracking");

    if (!(this->stateFlags2 & (PLAYER_STATE2_20 | PLAYER_STATE2_40))) {
        Actor* focusActor = this->focusActor;
        
        u32 immersive = recomp_get_config_u32("immersive_targeting"); 

        // MOD: Add "|| immersive" to the logic check
        if ((focusActor != NULL) &&
            (faster_aim || immersive || (play->actorCtx.attention.reticleSpinCounter != 0) || (this != GET_PLAYER(play))) &&
            (focusActor->id != ACTOR_OBJ_NOZOKI)) {
            
            s16 step = faster_aim ? 0x1F40 : 0xFA0;
            Math_ScaledStepToS(&this->actor.shape.rot.y, Math_Vec3f_Yaw(&this->actor.world.pos, &focusActor->focus.pos), step);
        } else if ((this->stateFlags1 & PLAYER_STATE1_PARALLEL) &&
                   !(this->stateFlags2 & (PLAYER_STATE2_20 | PLAYER_STATE2_40))) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, this->parallelYaw, 0xFA0);
        }
    } else if (!(this->stateFlags2 & PLAYER_STATE2_40)) {
        s16 step = faster_aim ? 0xFA0 : 0x7D0;
        Math_ScaledStepToS(&this->actor.shape.rot.y, this->yaw, step);
    }

    this->unk_B4C = this->actor.shape.rot.y - previousYaw;
}