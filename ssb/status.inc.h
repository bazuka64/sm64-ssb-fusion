void ftCommonAppealSetStatus(GObj* fighter_gobj)
{
    FTStruct* fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusAppeal, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    fp->motion_vars.flags.flag1 = 0;
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

void ftCommonWaitSetStatus(GObj* fighter_gobj)
{
    FTStruct* fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusWait, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
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

void ftAnimEndSetWait(GObj* fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
}

DObj* gcGetTreeDObjNext(DObj *dobj)
{
    if (dobj->child != NULL)
    {
        dobj = dobj->child;
    }
    else if (dobj->sib_next != NULL)
    {
        dobj = dobj->sib_next;
    }
    else while (TRUE)
    {
        if (dobj->parent == DOBJ_PARENT_NULL)
        {
            dobj = NULL;

            break;
        }
        else if (dobj->parent->sib_next != NULL)
        {
            dobj = dobj->parent->sib_next;

            break;
        }
        else dobj = dobj->parent;
    }
    return dobj;
}

void gcSetAnimSpeed(GObj *gobj, f32 anim_speed)
{
    DObj *dobj = DObjGetStruct(gobj);

    while (dobj != NULL)
    {
        dobj->anim_speed = anim_speed;
        dobj = gcGetTreeDObjNext(dobj);
    }
}