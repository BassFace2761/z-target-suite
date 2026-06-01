#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern void Attention_SetReticlePos(Attention *attention, s32 reticleNum, f32 x, f32 y, f32 z);
extern Gfx gLockOnReticleTriangleDL[];
extern Gfx gLockOnArrowDL[];

typedef struct
{
    Color_RGBA8 primary;
    Color_RGBA8 secondary;
} AttentionColor;

extern AttentionColor sAttentionColors[];

RECOMP_PATCH void Attention_Draw(Attention *attention, PlayState *play)
{
    Player* player = GET_PLAYER(play);
    Actor* actor;

    u32 immersiveEnabled = recomp_get_config_u32("immersive_targeting");

    if (player->stateFlags1 & (PLAYER_STATE1_2 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_200 |
                               PLAYER_STATE1_400 | PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000)) {
        return;
    }

    actor = attention->reticleActor;

    OPEN_DISPS(play->state.gfxCtx);

    if (attention->reticleFadeAlphaControl != 0)
    {
        LockOnReticle *reticle;
        s16 alpha = 255;
        f32 projectedPosScale = 1.0f;
        Vec3f projectedPos;
        s32 numReticles;
        f32 invW;
        s32 i;
        s32 curReticle;
        f32 lockOnScaleX;

        if (attention->reticleSpinCounter != 0)
        {
            numReticles = 1;
        }
        else
        {
            numReticles = ARRAY_COUNT(attention->lockOnReticles);
        }

        if (actor != NULL)
        {
            Math_Vec3f_Copy(&attention->reticlePos, &actor->focus.pos);
            projectedPosScale = (500.0f - attention->reticleRadius) / 420.0f;
        }
        else
        {
            attention->reticleFadeAlphaControl -= 120;

            if (attention->reticleFadeAlphaControl < 0)
            {
                attention->reticleFadeAlphaControl = 0;
            }
            alpha = attention->reticleFadeAlphaControl;
        }

        Actor_GetProjectedPos(play, &attention->reticlePos, &projectedPos, &invW);

        projectedPos.x = ((SCREEN_WIDTH / 2) * (projectedPos.x * invW)) * projectedPosScale;
        projectedPos.x = CLAMP(projectedPos.x, -SCREEN_WIDTH, SCREEN_WIDTH);

        projectedPos.y = ((SCREEN_HEIGHT / 2) * (projectedPos.y * invW)) * projectedPosScale;
        projectedPos.y = CLAMP(projectedPos.y, -SCREEN_HEIGHT, SCREEN_HEIGHT);

        projectedPos.z *= projectedPosScale;

        attention->curReticle--;

        if (attention->curReticle < 0)
        {
            attention->curReticle = ARRAY_COUNT(attention->lockOnReticles) - 1;
        }

        Attention_SetReticlePos(attention, attention->curReticle, projectedPos.x, projectedPos.y, projectedPos.z);

        if (!(player->stateFlags1 & PLAYER_STATE1_TALKING) || (actor != player->focusActor))
        {
            OVERLAY_DISP = Gfx_SetupDL(OVERLAY_DISP, SETUPDL_57);

            for (i = 0, curReticle = attention->curReticle; i < numReticles;
                 i++, curReticle = (curReticle + 1) % ARRAY_COUNT(attention->lockOnReticles))
            {
                reticle = &attention->lockOnReticles[curReticle];

                if (reticle->radius < 500.0f)
                {
                    s32 triangleIndex;

                    if (reticle->radius <= 120.0f)
                    {
                        lockOnScaleX = 0.15f;
                    }
                    else
                    {
                        lockOnScaleX = ((reticle->radius - 120.0f) * 0.001f) + 0.15f;
                    }

                    Matrix_Translate(reticle->pos.x, reticle->pos.y, 0.0f, MTXMODE_NEW);
                    Matrix_Scale(lockOnScaleX, 0.15f, 1.0f, MTXMODE_APPLY);

                    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, reticle->color.r, reticle->color.g, reticle->color.b,
                                    (u8)alpha);

                    Matrix_RotateZS(attention->reticleSpinCounter * 0x200, MTXMODE_APPLY);

                    for (triangleIndex = 0; triangleIndex < 4; triangleIndex++)
                    {
                        Matrix_RotateZS(0x10000 / 4, MTXMODE_APPLY);
                        Matrix_Push();
                        Matrix_Translate(reticle->radius, reticle->radius, 0.0f, MTXMODE_APPLY);
                        MATRIX_FINALIZE_AND_LOAD(OVERLAY_DISP++, play->state.gfxCtx);

                        if (!immersiveEnabled)
                        {
                            gSPDisplayList(OVERLAY_DISP++, gLockOnReticleTriangleDL);
                        }

                        Matrix_Pop();
                    }
                }

                alpha -= 255 / ARRAY_COUNT(attention->lockOnReticles);

                if (alpha < 0)
                {
                    alpha = 0;
                }
            }
        }
    }

    actor = attention->arrowHoverActor;

    if ((actor != NULL) && !(actor->flags & ACTOR_FLAG_LOCK_ON_DISABLED))
    {
        AttentionColor *attentionColor = &sAttentionColors[actor->category];

        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_7);

        Matrix_Translate(actor->focus.pos.x, actor->focus.pos.y + (actor->lockOnArrowOffset * actor->scale.y) + 17.0f,
                         actor->focus.pos.z, MTXMODE_NEW);
        Matrix_RotateYS(play->gameplayFrames * 0xBB8, MTXMODE_APPLY);
        Matrix_Scale((iREG(27) + 35) / 1000.0f, (iREG(28) + 60) / 1000.0f, (iREG(29) + 50) / 1000.0f, MTXMODE_APPLY);

        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, attentionColor->primary.r, attentionColor->primary.g,
                        attentionColor->primary.b, 255);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        if (!immersiveEnabled)
        {
            gSPDisplayList(POLY_XLU_DISP++, gLockOnArrowDL);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}