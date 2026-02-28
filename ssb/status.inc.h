#define FTCOMMON_ATTACK1_FOLLOWUP_FRAMES_DEFAULT 24.0F

#define ftCommonGroundCheckInterrupt(fighter_gobj)                    	\
(																		\
	/*(ftCommonSpecialNCheckInterruptCommon(fighter_gobj) != FALSE)  	|| 	\
	(ftCommonSpecialHiCheckInterruptCommon(fighter_gobj) != FALSE) 	||  \
	(ftCommonSpecialLwCheckInterruptCommon(fighter_gobj) != FALSE) 	||	\
	(ftCommonCatchCheckInterruptCommon(fighter_gobj) != FALSE) 		||  \
	(ftCommonAttackS4CheckInterruptCommon(fighter_gobj) != FALSE) 	|| 	\
	(ftCommonAttackHi4CheckInterruptCommon(fighter_gobj) != FALSE) 	|| 	\
	(ftCommonAttackLw4CheckInterruptCommon(fighter_gobj) != FALSE)	|| 	\
	(ftCommonAttackS3CheckInterruptCommon(fighter_gobj) != FALSE) 	||  \
	(ftCommonAttackHi3CheckInterruptCommon(fighter_gobj) != FALSE) 	||  \
	(ftCommonAttackLw3CheckInterruptCommon(fighter_gobj) != FALSE) 	||  \
	*/(ftCommonAttack1CheckInterruptCommon(fighter_gobj) != FALSE) 	|| 	\
    /*(ftCommonGuardOnCheckInterruptCommon(fighter_gobj) != FALSE) 	|| 	\
    */(ftCommonAppealCheckInterruptCommon(fighter_gobj) != FALSE) 	 ||	\
    (ftCommonKneeBendCheckInterruptCommon(fighter_gobj) != FALSE) 	|| 	\
    (ftCommonDashCheckInterruptCommon(fighter_gobj) != FALSE) 	|| \	
    /*(ftCommonPassCheckInterruptCommon(fighter_gobj) != FALSE) 		|| 	\
    (ftCommonDokanStartCheckInterruptCommon(fighter_gobj) != FALSE) || 	\
    (ftCommonSquatCheckInterruptCommon(fighter_gobj) != FALSE) 		|| 	\
    (ftCommonTurnCheckInterruptCommon(fighter_gobj) != FALSE) 		|| 	\
    (ftCommonWalkCheckInterruptCommon(fighter_gobj) != FALSE) 			\
*/FALSE)

void ftCommonAttack11SetStatus(GObj* fighter_gobj)
{
    FTStruct* fp = ftGetStruct(fighter_gobj);
    FTAttributes* attr = fp->attr;
    ftMainSetStatus(fighter_gobj, nFTCommonStatusAttack11, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->attack1_followup_frames = attr->attack1_followup_frames;
}

void ftCommonAttack12SetStatus(GObj* fighter_gobj)
{
    FTStruct* fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusAttack12, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    fp->attack1_followup_frames = FTCOMMON_ATTACK1_FOLLOWUP_FRAMES_DEFAULT;
}

void ftCommonAttack13SetStatus(GObj* fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTCommonStatusWait, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

sb32 ftCommonAttack1CheckInterruptCommon(GObj* fighter_gobj)
{
    FTStruct* fp = ftGetStruct(fighter_gobj);
    FTAttributes* attr = fp->attr;

    if (fp->input.pl.button_tap & fp->input.button_mask_a)
    {
        if (fp->attack1_followup_frames != 0.0F) {
            // switch (fp->attack1_status_id)
            // {
            // case nFTCommonStatusAttack11:
            //     if (attr->is_have_attack12)
            //     {
            //         ftCommonAttack12SetStatus(fighter_gobj);

            //         fp->attack1_input_count++;

            //         return TRUE;
            //     }
            //     break;

            // case nFTCommonStatusAttack12:
            //     // if (ftCommonAttack13CheckFighterKind(fp))
            // {
            //     ftCommonAttack13SetStatus(fighter_gobj);

            //     return TRUE;
            // }
            // break;
            // }
        }
        else if (attr->is_have_attack11) {
            ftCommonAttack11SetStatus(fighter_gobj);
            return TRUE;
        }
    }
    if (fp->attack1_followup_frames != 0.0F)
    {
        fp->attack1_followup_frames -= DObjGetStruct(fighter_gobj)->anim_speed;
    }
    return FALSE;
}

void ftCommonAppealSetStatus(GObj* fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTCommonStatusAppeal, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

sb32 ftCommonAppealCheckInterruptCommon(GObj* fighter_gobj)
{
    FTStruct* fp = ftGetStruct(fighter_gobj);

    if (fp->input.pl.button_tap & fp->input.button_mask_l)
    {
        ftCommonAppealSetStatus(fighter_gobj);

        return TRUE;
    }
    else return FALSE;
}

void ftCommonWaitProcInterrupt(GObj* fighter_gobj)
{
    !(ftCommonGroundCheckInterrupt(fighter_gobj));
}

sb32 ftAnimEndCheckSetStatus(GObj* fighter_gobj, void (*proc_status)(GObj*))
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        proc_status(fighter_gobj);

        return TRUE;
    }
    else return FALSE;
}

void ftCommonWaitSetStatus(GObj* fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTCommonStatusWait, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

void ftAnimEndSetWait(GObj* fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
}

void ftCommonAttack11ProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftAnimEndSetWait(fighter_gobj);
}
sb32 ftCommonAttack12CheckGoto(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    if (fp->attack1_followup_frames != 0.0F)
    {
        fp->attack1_followup_frames -= DObjGetStruct(fighter_gobj)->anim_speed;

        if ((fp->input.pl.button_tap & fp->input.button_mask_a) && (attr->is_have_attack12))
        {
            if (fp->motion_vars.flags.flag1 != 0)
            {
                ftCommonAttack12SetStatus(fighter_gobj);

                return TRUE;
            }
        }
    }
    return FALSE;
}
void ftCommonAttack11ProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
// ftCommonAttack12CheckGoto(fighter_gobj);
}

#define FTCOMMON_DASH_STICK_RANGE_MIN 56
#define FTCOMMON_DASH_BUFFER_TICS_MAX       3 

void ftCommonDashSetStatus(GObj *fighter_gobj, u32 flag)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusDash, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    // ftMainPlayAnimEventsAll(fighter_gobj);

    fp->physics.vel_ground.x = fp->attr->dash_speed;
    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->motion_vars.flags.flag1 = flag;
}

#define ABS(x) ((x) < 0 ? -(x) : (x))

void ftParamSetStickLR(FTStruct *fp)
{
    fp->lr = (fp->input.pl.stick_range.x >= 0) ? +1 : -1;
}

sb32 ftCommonDashCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((ABS(fp->input.pl.stick_range.x) >= FTCOMMON_DASH_STICK_RANGE_MIN) && (fp->tap_stick_x < FTCOMMON_DASH_BUFFER_TICS_MAX))
    {
        // if ((fp->input.pl.stick_range.x * fp->lr) < 0)
        // {
        //     ftCommonTurnSetStatusInvertLR(fighter_gobj);

        //     return TRUE;
        // }
        ftParamSetStickLR(fp);
        ftCommonDashSetStatus(fighter_gobj, 1);

        return TRUE;
    }
    return FALSE;
}

void ftCommonDashProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->physics.vel_ground.x *= 0.75F;

        ftCommonWaitSetStatus(fighter_gobj);
    }
}

void ftPhysicsSetGroundVelFriction(FTStruct *fp, f32 friction)
{
    if (fp->physics.vel_ground.x < 0.0F)
    {
        fp->physics.vel_ground.x += friction;

        if (fp->physics.vel_ground.x > 0.0F)
        {
            fp->physics.vel_ground.x = 0.0F;
        }
    }
    else
    {
        fp->physics.vel_ground.x -= friction;

        if (fp->physics.vel_ground.x < 0.0F)
        {
            fp->physics.vel_ground.x = 0.0F;
        }
    }
}

void ftPhysicsApplyGroundVelFriction(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftPhysicsSetGroundVelFriction(fp, 4.0F * attr->traction);
    // ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

#define FTCOMMON_KNEEBEND_INPUT_TYPE_NONE 0
#define FTCOMMON_KNEEBEND_INPUT_TYPE_STICK 1
#define FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON 2
#define FTCOMMON_KNEEBEND_STICK_RANGE_MIN 53		// Minimum stick Y range required for stick jump
#define FTCOMMON_KNEEBEND_BUFFER_TICS_MAX 3
sb32 ftCommonKneeBendCheckButtonTap(FTStruct *fp)
{
    if (fp->input.pl.button_tap & (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS))
    {
        return TRUE;
    }
    else return FALSE;
}
s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp)
{
    if ((fp->input.pl.stick_range.y >= FTCOMMON_KNEEBEND_STICK_RANGE_MIN) && (fp->tap_stick_y <= FTCOMMON_KNEEBEND_BUFFER_TICS_MAX))
    {
        return FTCOMMON_KNEEBEND_INPUT_TYPE_STICK;
    }
    else if (ftCommonKneeBendCheckButtonTap(fp) != FALSE)
    {
        return FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON;
    }
    else return FTCOMMON_KNEEBEND_INPUT_TYPE_NONE;
}

void ftCommonKneeBendSetStatusParam(GObj *fighter_gobj, s32 status_id, s32 input_source)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    // fp->status_vars.common.kneebend.jump_force = fp->input.pl.stick_range.y;
    // fp->status_vars.common.kneebend.anim_frame = 0.0F;
    // fp->status_vars.common.kneebend.input_source = input_source;
    // fp->status_vars.common.kneebend.is_shorthop = FALSE;

    fp->is_special_interrupt = TRUE;
}

void ftCommonKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
    ftCommonKneeBendSetStatusParam(fighter_gobj, nFTCommonStatusKneeBend, input_source);
}

sb32 ftCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 input_source;

    input_source = ftCommonKneeBendGetInputTypeCommon(fp);

    if (input_source != FTCOMMON_KNEEBEND_INPUT_TYPE_NONE)
    {
        ftCommonKneeBendSetStatus(fighter_gobj, input_source);

        return TRUE;
    }
    else return FALSE;
}

void ftCommonJumpSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 status_id;
    s32 vel_x, vel_y;

    // mpCommonSetFighterAir(fp);

    // status_id = ((fp->input.pl.stick_range.x * fp->lr) > FTCOMMON_KNEEBEND_JUMP_F_OR_B_RANGE) ? nFTCommonStatusJumpF : nFTCommonStatusJumpB;
    status_id = nFTCommonStatusJumpF;

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    // switch (fp->status_vars.common.kneebend.input_source)
    // {
    // case FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON:
    //     ftCommonJumpGetJumpForceButton(fp->input.pl.stick_range.x, &vel_x, &vel_y, fp->status_vars.common.kneebend.is_shorthop);
    //     break;

    // case FTCOMMON_KNEEBEND_INPUT_TYPE_STICK:
    // default:
    //     vel_x = fp->input.pl.stick_range.x;
    //     vel_y = fp->status_vars.common.kneebend.jump_force;

    //     if (vel_y < FTCOMMON_KNEEBEND_STICK_RANGE_MIN)
    //     {
    //         vel_y = FTCOMMON_KNEEBEND_STICK_RANGE_MIN;
    //     }
    //     break;
    // }
    vel_y = 1;
    vel_x = 1;
    fp->physics.vel_air.y = (vel_y * attr->jump_height_mul) + attr->jump_height_base;
    fp->physics.vel_air.x = vel_x * attr->jump_vel_x;

    // fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    // fp->is_special_interrupt = TRUE;
}

void ftCommonKneeBendProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    //fp->status_vars.common.kneebend.anim_frame += DObjGetStruct(fighter_gobj)->anim_speed;

    // if 
    // (
    //     (fp->status_vars.common.kneebend.input_source == FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON)&&
    //     (fp->status_vars.common.kneebend.anim_frame <= FTCOMMON_KNEEBEND_SHORTHOP_FRAMES)    &&
    //     (fp->input.pl.button_release & (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS))
    // )
    // {
    //     fp->status_vars.common.kneebend.is_shorthop = TRUE;
    // }
    // if (attr->kneebend_anim_length <= fp->status_vars.common.kneebend.anim_frame) 
    {
        ftCommonJumpSetStatus(fighter_gobj);
    }
}

void ftPhysicsApplyGravityClampTVel(FTStruct *fp, f32 gravity, f32 tvel)
{
    fp->physics.vel_air.y -= gravity;

    if (fp->physics.vel_air.y < -tvel)
    {
        fp->physics.vel_air.y = -tvel;
    }
}
void ftPhysicsApplyGravityDefault(FTStruct *fp, FTAttributes *attr)
{
    ftPhysicsApplyGravityClampTVel(fp, attr->gravity, attr->tvel_base);
}
void ftPhysicsApplyAirVelDriftFastFall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    
    // ftPhysicsCheckSetFastFall(fp);
    
    //(fp->is_fastfall) ? ftPhysicsApplyFastFall(fp, attr) : ftPhysicsApplyGravityDefault(fp, attr);
    ftPhysicsApplyGravityDefault(fp, attr);
    
    // if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    // {
    //     ftPhysicsClampAirVelXStickDefault(fp, attr);
    //     ftPhysicsApplyAirVelXFriction(fp, attr);
    // }
}

void proc_update_1(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
}