#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern s16 func_80832754(Player* this, s32 arg1);

RECOMP_PATCH s32 func_8083C62C(Player* this, s32 arg1) {
    Actor* focusActor = this->focusActor;
    Vec3f headPos;
    s16 pitchTarget;
    s16 yawTarget;

    u32 fasterAimEnabled = recomp_get_config_u32("faster_aim_tracker");

    headPos.x = this->actor.world.pos.x;
    headPos.y = this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 3.0f;
    headPos.z = this->actor.world.pos.z;

    pitchTarget = Math_Vec3f_Pitch(&headPos, &focusActor->focus.pos);
    yawTarget = Math_Vec3f_Yaw(&headPos, &focusActor->focus.pos);

    s16 stepFraction = fasterAimEnabled ? 2 : 4;
    s16 maxStep = fasterAimEnabled ? 0x7FFF : 0x2710;

    Math_SmoothStepToS(&this->actor.focus.rot.y, yawTarget, stepFraction, maxStep, 0);
    Math_SmoothStepToS(&this->actor.focus.rot.x, pitchTarget, stepFraction, maxStep, 0);

    this->unk_AA6_rotFlags |= UNKAA6_ROT_FOCUS_Y;

    return func_80832754(this, arg1);
}