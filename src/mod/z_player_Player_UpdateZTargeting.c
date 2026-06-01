#include "global.h"
#include "modding.h"
#include "recompconfig.h"

extern Input* sPlayerControlInput;
extern s32 Player_IsTalking(PlayState* play);
extern void Player_SetParallel(Player* this);

extern void Player_Action_95(Player *this, PlayState *play);

RECOMP_PATCH void Player_UpdateZTargeting(Player *this, PlayState *play)
{
    s32 ignoreLeash = false;
    Actor *nextLockOnActor;
    s32 zButtonHeld = CHECK_BTN_ALL(sPlayerControlInput->cur.button, BTN_Z);
    s32 isTalking;
    s32 usingHoldTargeting;

    // Check our suite's toggles!
    u32 tpAutoRetargetEnabled = recomp_get_config_u32("tp_auto_retarget");
    u32 instantTargetEnabled = recomp_get_config_u32("instant_targeting");

    if (!zButtonHeld)
    {
        this->stateFlags1 &= ~PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE;
    }

    if ((play->csCtx.state != CS_STATE_IDLE) || (this->csAction != PLAYER_CSACTION_NONE) ||
        (this->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_20000000)) ||
        (this->stateFlags3 & PLAYER_STATE3_FLYING_WITH_HOOKSHOT))
    {
        this->zTargetActiveTimer = 0;
    }
    else if (zButtonHeld || (this->stateFlags2 & PLAYER_STATE2_LOCK_ON_WITH_SWITCH) ||
             (this->autoLockOnActor != NULL))
    {
        if (this->zTargetActiveTimer <= 5)
        {
            this->zTargetActiveTimer = 5;
        }
        else
        {
            this->zTargetActiveTimer--;
        }
    }
    else if (this->stateFlags1 & PLAYER_STATE1_PARALLEL)
    {
        this->zTargetActiveTimer = 0;
    }
    else if (this->zTargetActiveTimer != 0)
    {
        // Instant Target Release
        if (instantTargetEnabled)
        {
            this->zTargetActiveTimer = 0;
        }
        else
        {
            this->zTargetActiveTimer--;
        }
    }

    if (this->zTargetActiveTimer >= 6)
    {
        ignoreLeash = true;
    }

    isTalking = Player_IsTalking(play);

    if (isTalking || (this->zTargetActiveTimer != 0) ||
        (this->stateFlags1 & (PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_ZORA_BOOMERANG_THROWN)))
    {
        if (!isTalking)
        {
            if (!(this->stateFlags1 & PLAYER_STATE1_ZORA_BOOMERANG_THROWN) &&
                ((this->heldItemAction != PLAYER_IA_FISHING_ROD) || (this->unk_B28 == 0)) &&
                CHECK_BTN_ALL(sPlayerControlInput->press.button, BTN_Z))
            {

                if (this == GET_PLAYER(play))
                {
                    nextLockOnActor = play->actorCtx.attention.tatlHoverActor;
                }
                else
                {
                    nextLockOnActor = &GET_PLAYER(play)->actor;
                }

                usingHoldTargeting = (gSaveContext.options.zTargetSetting != 0) || (this != GET_PLAYER(play));

                this->stateFlags1 |= PLAYER_STATE1_Z_TARGETING;

                if ((this->currentMask != PLAYER_MASK_GIANT) && (nextLockOnActor != NULL) &&
                    !(nextLockOnActor->flags & ACTOR_FLAG_LOCK_ON_DISABLED) &&
                    !(this->stateFlags3 & (PLAYER_STATE3_200 | PLAYER_STATE3_2000)))
                {

                    if ((nextLockOnActor == this->focusActor) && (this == GET_PLAYER(play)))
                    {
                        nextLockOnActor = play->actorCtx.attention.arrowHoverActor;
                    }

                    if ((nextLockOnActor != NULL) && (((nextLockOnActor != this->focusActor)) ||
                                                      (nextLockOnActor->flags & ACTOR_FLAG_FOCUS_ACTOR_REFINDABLE)))
                    {

                        nextLockOnActor->flags &= ~ACTOR_FLAG_FOCUS_ACTOR_REFINDABLE;

                        if (!usingHoldTargeting)
                        {
                            this->stateFlags2 |= PLAYER_STATE2_LOCK_ON_WITH_SWITCH;
                        }

                        this->focusActor = nextLockOnActor;

                        // ==========================================
                        // CONFIG CHECK: Instant Lock-On Engagement
                        // ==========================================
                        if (instantTargetEnabled)
                        {
                            this->zTargetActiveTimer = 5;
                        }
                        else
                        {
                            this->zTargetActiveTimer = 15;
                        }

                        this->stateFlags2 &= ~(PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER | PLAYER_STATE2_200000);
                    }
                    else if (!usingHoldTargeting)
                    {
                        Player_ReleaseLockOn(this);

                        // Instant Target Release (Switch Mode)
                        if (instantTargetEnabled)
                        {
                            this->zTargetActiveTimer = 0;
                        }
                    }
                    this->stateFlags1 &= ~PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE;
                }
                else
                {
                    if (!(this->stateFlags1 & (PLAYER_STATE1_PARALLEL | PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE)) &&
                        (Player_Action_95 != this->actionFunc))
                    {
                        Player_SetParallel(this);
                    }
                }
            }

            if (this->focusActor != NULL)
            {
                if ((this == GET_PLAYER(play)) && (this->focusActor != this->autoLockOnActor) &&
                    Attention_ShouldReleaseLockOn(this->focusActor, this, ignoreLeash))
                {

                    bool targetDied = (this->focusActor->update == NULL) || !(this->focusActor->flags & ACTOR_FLAG_ATTENTION_ENABLED);
                    bool swappedTarget = false;

                    // TP Auto-Retarget Logic
                    if (tpAutoRetargetEnabled && targetDied)
                    {
                        bool holdTargetingActive = (gSaveContext.options.zTargetSetting != 0);

                        if (!holdTargetingActive || zButtonHeld)
                        {
                            Actor *nextTarget = play->actorCtx.attention.arrowHoverActor;

                            if (nextTarget != NULL && !(nextTarget->flags & ACTOR_FLAG_LOCK_ON_DISABLED) &&
                                CHECK_FLAG_ALL(nextTarget->flags, ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE))
                            {

                                this->focusActor = nextTarget;

                                // ==========================================
                                // CONFIG CHECK: Instant Lock-On Engagement (Auto-Retarget)
                                // ==========================================
                                if (instantTargetEnabled)
                                {
                                    this->zTargetActiveTimer = 5;
                                }
                                else
                                {
                                    this->zTargetActiveTimer = 15;
                                }

                                swappedTarget = true;
                            }
                        }
                    }

                    if (!swappedTarget)
                    {
                        Player_ReleaseLockOn(this);
                        this->stateFlags1 |= PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE;

                        // Instant Target Release (Distance/Death break)
                        if (instantTargetEnabled)
                        {
                            this->zTargetActiveTimer = 0;
                        }
                    }
                }
                else if (this->focusActor != NULL)
                {
                    this->focusActor->targetPriority = 0x28;
                }
            }
            else if (this->autoLockOnActor != NULL)
            {
                this->focusActor = this->autoLockOnActor;
            }
        }

        if ((this->focusActor != NULL) && !(this->stateFlags3 & (PLAYER_STATE3_200 | PLAYER_STATE3_2000)))
        {
            this->stateFlags1 &= ~(PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL);

            if ((this->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) ||
                !CHECK_FLAG_ALL(this->focusActor->flags, ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE))
            {
                this->stateFlags1 |= PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS;
            }
        }
        else if (this->stateFlags1 & PLAYER_STATE1_PARALLEL)
        {
            this->stateFlags2 &= ~PLAYER_STATE2_LOCK_ON_WITH_SWITCH;
        }
        else
        {
            Player_ClearZTargeting(this);
        }
    }
    else
    {
        Player_ClearZTargeting(this);
    }
}