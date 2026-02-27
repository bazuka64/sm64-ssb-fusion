void ftMainSetStatus(GObj* fighter_gobj, s32 status_id, f32 frame_begin, f32 anim_speed, u32 flags) {
    FTStatusDesc* status_struct;
    FTStatusDesc* status_desc;
    FTStruct* fp = &fighter;
    FTMotionDesc* motion_desc;
    FTMotionDescArray* script_array = fp->data->mainmotion;
    s32 motion_id;

    if (status_id == nFTCommonStatusAppeal) {
        status_desc = &appealDesc;
    }
    else {
        status_desc = &waitDesc;
    }
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

void ftMainProcPhysicsMap(GObj* fighter_gobj) {

}