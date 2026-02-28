void ftMainSetStatus(GObj* fighter_gobj, s32 status_id, f32 frame_begin, f32 anim_speed, u32 flags) {
    FTStatusDesc* status_struct;
    FTStatusDesc* status_desc;
    FTStruct* fp = &fighter;
    FTMotionDesc* motion_desc;
    FTMotionDescArray* script_array = fp->data->mainmotion;
    s32 motion_id;

    status_desc = &dFTCommonActionStatusDescs[status_id - nFTCommonStatusActionStart];
    motion_id = status_desc->mflags.motion_id;

    motion_desc = &script_array->motion_desc[motion_id];
    lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, (void*)fp->figatree_heap);
    lbCommonAddFighterPartsFigatree(fp->joints[nFTPartsJointTopN]->child, fp->figatree, 1/*応急処置0だと変になる*/);
    if (anim_speed != DObjGetStruct(fighter_gobj)->anim_speed)
    {
        gcSetAnimSpeed(fighter_gobj, anim_speed);
    }

    fp->proc_update = status_desc->proc_update;
    fp->proc_interrupt = status_desc->proc_interrupt;
    fp->proc_physics = status_desc->proc_physics;
    fp->proc_map = status_desc->proc_map;
}

#define I_CONTROLLER_RANGE_MAX 80									// Maximum control stick range in integer


void ftMainProcUpdateInterrupt(GObj* fighter_gobj) {
    FTStruct* this_fp = ftGetStruct(fighter_gobj);
    SYController* controller;
    u16 button_tap_mask;
    u16 button_hold;
    FTPlayerInput* pl;

    {
        pl = &this_fp->input.pl;
        pl->stick_prev.x = pl->stick_range.x;
        pl->stick_prev.y = pl->stick_range.y;

        controller = this_fp->input.controller;

        // mario to ssb controlelr
        controller->button_hold = gControllers[0].buttonDown;
        controller->stick_range.x = gControllers[0].rawStickX;
        controller->stick_range.y = gControllers[0].rawStickY;

        button_hold = controller->button_hold;

        if (button_hold & R_TRIG)
        {
            button_hold |= (A_BUTTON | Z_TRIG);
        }
        pl->stick_range.x = controller->stick_range.x;
        pl->stick_range.y = controller->stick_range.y;

        button_tap_mask = (button_hold ^ pl->button_hold) & button_hold;

        pl->button_tap = button_tap_mask;

        button_tap_mask = (button_hold ^ pl->button_hold) & pl->button_hold;

        pl->button_release = button_tap_mask;

        pl->button_hold = button_hold;

        if (pl->stick_range.x > I_CONTROLLER_RANGE_MAX)
        {
            pl->stick_range.x = I_CONTROLLER_RANGE_MAX;
        }
        if (pl->stick_range.x < -I_CONTROLLER_RANGE_MAX)
        {
            pl->stick_range.x = -I_CONTROLLER_RANGE_MAX;
        }
        if (pl->stick_range.y > I_CONTROLLER_RANGE_MAX)
        {
            pl->stick_range.y = I_CONTROLLER_RANGE_MAX;
        }
        if (pl->stick_range.y < -I_CONTROLLER_RANGE_MAX)
        {
            pl->stick_range.y = -I_CONTROLLER_RANGE_MAX;
        }

        if (pl->stick_range.x >= 20)
        {
            if (pl->stick_prev.x >= 20)
            {
                this_fp->tap_stick_x++, this_fp->hold_stick_x++;
            }
            else this_fp->tap_stick_x = this_fp->hold_stick_x = 1;
        }
        else if (pl->stick_range.x <= -20)
        {
            if (pl->stick_prev.x <= -20)
            {
                this_fp->tap_stick_x++, this_fp->hold_stick_x++;
            }
            else this_fp->tap_stick_x = this_fp->hold_stick_x = 1;
        }
        else this_fp->tap_stick_x = this_fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;

        if (this_fp->tap_stick_x > FTINPUT_STICKBUFFER_TICS_MAX)
        {
            this_fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        }
        if (this_fp->hold_stick_x > FTINPUT_STICKBUFFER_TICS_MAX)
        {
            this_fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        }
        if (pl->stick_range.y >= 20)
        {
            if (pl->stick_prev.y >= 20)
            {
                this_fp->tap_stick_y++, this_fp->hold_stick_y++;
            }
            else this_fp->tap_stick_y = this_fp->hold_stick_y = 1;
        }
        else if (pl->stick_range.y <= -20)
        {
            if (pl->stick_prev.y <= -20)
            {
                this_fp->tap_stick_y++, this_fp->hold_stick_y++;
            }
            else this_fp->tap_stick_y = this_fp->hold_stick_y = 1;
        }
        else this_fp->tap_stick_y = this_fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

        if (this_fp->tap_stick_y > FTINPUT_STICKBUFFER_TICS_MAX)
        {
            this_fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        }
        if (this_fp->hold_stick_y > FTINPUT_STICKBUFFER_TICS_MAX)
        {
            this_fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        }
    }

    ftParamUpdateAnimKeys(&gobj);

    if (this_fp->proc_update != NULL)
    {
        this_fp->proc_update(fighter_gobj);
    }
    if (this_fp->proc_interrupt != NULL)
    {
        this_fp->proc_interrupt(fighter_gobj);
    }
}

#define SCALE 0.5F

void ftMainProcPhysicsMap(GObj* fighter_gobj, Vec3fArray pos) {
    FTStruct* fp = ftGetStruct(fighter_gobj);

    if (fp->proc_physics != NULL)
    {
        fp->proc_physics(fighter_gobj);
    }

    pos[0] += fp->physics.vel_ground.x * fp->lr * SCALE;
    pos[1] += fp->physics.vel_air.y * SCALE;

    if (pos[1] < 260) {
        pos[1] = 260;
        fp->physics.vel_air.y = 0;
        ftMainSetStatus(fighter_gobj, nFTCommonStatusWait, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    }
}

