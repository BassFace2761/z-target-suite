#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern u32 recomp_get_config_u32(const char *name);

// Let the compiler know about this internal z_player.c function
extern s32 Player_CalcSpeedAndYawFromControlStick(PlayState *play, Player *this, f32 *outSpeedTarget, s16 *outYawTarget, f32 speedMode);

RECOMP_PATCH s32 Player_GetMovementSpeedAndYaw(Player *this, f32 *outSpeedTarget, s16 *outYawTarget, f32 speedMode, PlayState *play)
{
    if (!Player_CalcSpeedAndYawFromControlStick(play, this, outSpeedTarget, outYawTarget, speedMode))
    {
        *outYawTarget = this->actor.shape.rot.y;

        if (this->focusActor != NULL)
        {
            u32 fasterAim = recomp_get_config_u32("faster_aim_tracking");
            u32 immersive = recomp_get_config_u32("immersive_targeting");

            if ((fasterAim || immersive || (play->actorCtx.attention.reticleSpinCounter != 0)) && !(this->stateFlags2 & PLAYER_STATE2_40))
            {
                *outYawTarget = Math_Vec3f_Yaw(&this->actor.world.pos, &this->focusActor->focus.pos);
            }
        }
        else if (Player_FriendlyLockOnOrParallel(this))
        {
            *outYawTarget = this->parallelYaw;
        }

        return false;
    }

    *outYawTarget += Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
    return true;
}