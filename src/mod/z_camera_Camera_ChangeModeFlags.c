#include "global.h"
#include "modding.h"
#include "recompconfig.h"

#define CAM_CHANGE_MODE_0 (1 << 0)
#define CAM_CHANGE_MODE_1 (1 << 1)
#define CAM_CHANGE_MODE_BATTLE (1 << 2)
#define CAM_CHANGE_MODE_FOLLOW_TARGET (1 << 3)
#define CAM_CHANGE_MODE_4 (1 << 4)
#define CAM_CHANGE_MODE_FIRST_PERSON (1 << 5)

typedef struct {
    s16 val;
    s16 param;
} CameraModeValue;

typedef struct {
    s16 funcId;
    s16 numValues;
    CameraModeValue* values;
} CameraMode;

typedef struct {
    u32 validModes;
    u32 flags;
    CameraMode* cameraModes;
} CameraSetting;

extern void Camera_ResetActionFuncState(Camera* camera, s16 mode);
extern s32 func_800DF498(Camera* camera);
extern CameraSetting sCameraSettings[];

RECOMP_PATCH s32 Camera_ChangeModeFlags(Camera* camera, s16 mode, u8 forceChange) {
    static s32 sModeChangeFlags = 0;
    
    // Check our suite's toggle!
    u32 immersiveTargetingOn = recomp_get_config_u32("immersive_targeting");

    if ((camera->setting == CAM_SET_TELESCOPE) && ((mode == CAM_MODE_FIRSTPERSON) || (mode == CAM_MODE_DEKUHIDE))) {
        forceChange = true;
    }

    // Mode change rejected by flag
    if ((camera->stateFlags & CAM_STATE_DISABLE_MODE_CHANGE) && !forceChange) {
        camera->behaviorFlags |= CAM_BEHAVIOR_MODE_VALID;
        return -1;
    }

    // Mode change rejected by validModes
    if (!(sCameraSettings[camera->setting].validModes & (1 << mode))) {
        if (camera->mode != CAM_MODE_NORMAL) {
            camera->mode = CAM_MODE_NORMAL;
            Camera_ResetActionFuncState(camera, camera->mode);
            func_800DF498(camera);
            return mode | 0xC0000000;
        } else {
            camera->behaviorFlags |= CAM_BEHAVIOR_MODE_VALID;
            camera->behaviorFlags |= CAM_BEHAVIOR_MODE_1;
            return 0;
        }
    }

    // Mode change rejected due to mode already being set. (otherwise, reset mode)
    if ((mode == camera->mode) && !forceChange) {
        camera->behaviorFlags |= CAM_BEHAVIOR_MODE_VALID;
        return -1;
    }

    camera->behaviorFlags |= CAM_BEHAVIOR_MODE_VALID;
    camera->behaviorFlags |= CAM_BEHAVIOR_MODE_1;

    Camera_ResetActionFuncState(camera, mode);

    sModeChangeFlags = 0;

    // Process Requested Camera Mode
    switch (mode) {
        case CAM_MODE_FIRSTPERSON:
            sModeChangeFlags = CAM_CHANGE_MODE_FIRST_PERSON;
            break;

        case CAM_MODE_BATTLE:
            sModeChangeFlags = CAM_CHANGE_MODE_BATTLE;
            break;

        case CAM_MODE_FOLLOWTARGET:
            if ((camera->target != NULL) && (camera->target->id != ACTOR_EN_BOOM)) {
                sModeChangeFlags = CAM_CHANGE_MODE_FOLLOW_TARGET;
            }
            break;

        case CAM_MODE_BOWARROWZ:
        case CAM_MODE_TARGET:
        case CAM_MODE_TALK:
        case CAM_MODE_HANGZ:
        case CAM_MODE_PUSHPULL:
            sModeChangeFlags = CAM_CHANGE_MODE_1;
            break;

        case CAM_MODE_NORMAL:
        case CAM_MODE_HANG:
            sModeChangeFlags = CAM_CHANGE_MODE_4;
            break;

        default:
            break;
    }

    // Process Current Camera Mode
    switch (camera->mode) {
        case CAM_MODE_FIRSTPERSON:
            if (sModeChangeFlags & CAM_CHANGE_MODE_FIRST_PERSON) {
                camera->animState = 10;
            }
            break;

        case CAM_MODE_JUMP:
        case CAM_MODE_HANG:
            if (sModeChangeFlags & CAM_CHANGE_MODE_4) {
                camera->animState = 20;
            }
            sModeChangeFlags |= CAM_CHANGE_MODE_0;
            break;

        case CAM_MODE_CHARGE:
            if (sModeChangeFlags & CAM_CHANGE_MODE_4) {
                camera->animState = 20;
            }
            sModeChangeFlags |= CAM_CHANGE_MODE_0;
            break;

        case CAM_MODE_FOLLOWTARGET:
            if (sModeChangeFlags & CAM_CHANGE_MODE_FOLLOW_TARGET) {
                camera->animState = 10;
            }
            sModeChangeFlags |= CAM_CHANGE_MODE_0;
            break;

        case CAM_MODE_BATTLE:
            if (sModeChangeFlags & CAM_CHANGE_MODE_BATTLE) {
                camera->animState = 10;
            }
            sModeChangeFlags |= 1;
            break;

        case CAM_MODE_BOWARROWZ:
        case CAM_MODE_HANGZ:
        case CAM_MODE_PUSHPULL:
            sModeChangeFlags |= CAM_CHANGE_MODE_0;
            break;

        case CAM_MODE_NORMAL:
            if (sModeChangeFlags & CAM_CHANGE_MODE_4) {
                camera->animState = 20;
            }
            break;

        default:
            break;
    }

    sModeChangeFlags &= ~CAM_CHANGE_MODE_4;

    // ==========================================
    // CONFIG CHECK: Z-Pressing Sfx
    // Only play audio if Immersive Targeting is OFF
    // ==========================================
    if ((camera->status == CAM_STATUS_ACTIVE) && !immersiveTargetingOn) {
        switch (sModeChangeFlags) {
            case CAM_CHANGE_MODE_0:
                Audio_PlaySfx(0);
                break;

            case CAM_CHANGE_MODE_1:
                if (camera->play->roomCtx.curRoom.type == ROOM_TYPE_DUNGEON) {
                    Audio_PlaySfx(NA_SE_SY_ATTENTION_URGENCY);
                } else {

                    Audio_PlaySfx(NA_SE_SY_ATTENTION_ON);
                }
                break;

            case CAM_CHANGE_MODE_BATTLE:
                Audio_PlaySfx(NA_SE_SY_ATTENTION_URGENCY);
                break;

            case CAM_CHANGE_MODE_FOLLOW_TARGET:
                Audio_PlaySfx(NA_SE_SY_ATTENTION_ON);
                break;

            default:
                break;
        }
    }

    func_800DF498(camera);
    camera->mode = mode;

    return mode | 0x80000000;
}