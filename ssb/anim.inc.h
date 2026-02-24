void* ftManagerAllocFigatreeHeapKind(s32 fkind)
{
    FTData* data = &dFTNessData;

    // return syTaskmanMalloc(data->file_anim_size, 0x10);
    return syTaskmanMalloc(0x4000/*アニメファイルサイズ要計算*/, 0x10);
}

void* lbRelocGetForceExternHeapFile(u32 id, void* heap)
{
    sLBRelocExternFileHeap = heap;
    sLBRelocInternBuffer.force_status_buffer_num = 0;

    return lbRelocGetForceExternBufferFile(id);
}

DObj* lbCommonGetTreeDObjNextFromRoot(DObj* a, DObj* b)
{
    if (a->child != NULL)
    {
        a = a->child;
    }
    else if (a == b)
    {
        a = NULL;
    }
    else if (a->sib_next != NULL)
    {
        a = a->sib_next;
    }
    else while (TRUE)
    {
        if (a->parent == b)
        {
            a = NULL;

            break;
        }
        else if (a->parent->sib_next != NULL)
        {
            a = a->parent->sib_next;

            break;
        }
        else a = a->parent;
    }
    return a;
}

void gcAddDObjAnimJoint(DObj* dobj, AObjEvent32* anim_joint, f32 anim_frame)
{
    AObj* aobj = dobj->aobj;

    while (aobj != NULL)
    {
        aobj->kind = nGCAnimKindNone;
        aobj = aobj->next;
    }
    dobj->anim_joint.event32 = anim_joint;
    dobj->anim_wait = AOBJ_ANIM_CHANGED;
    dobj->anim_frame = anim_frame;
}

void lbCommonAddFighterPartsFigatree(DObj* root_dobj, void** figatree, f32 anim_frame)
{
    DObj* current_dobj = root_dobj;

    root_dobj->parent_gobj->anim_frame = anim_frame;

    while (current_dobj != NULL)
    {
        void* anim = *figatree;
        FTParts* parts = current_dobj->user_data.p;

        if (anim != NULL)
        {
            gcAddDObjAnimJoint(current_dobj, anim, anim_frame);

            // parts->is_have_anim = TRUE;
        }
        else
        {
            current_dobj->anim_wait = AOBJ_ANIM_NULL;

            // parts->is_have_anim = FALSE;
        }
        figatree++;

        current_dobj = lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

#define ftGetStruct(fighter_gobj) 	((FTStruct*)(fighter_gobj)->user_data.p)
#define AObjAnimAdvance(script) \
((script)++)

AObj *sGCAnimHead;
u32 sGCAnimsActiveNum;

AObj* gcGetAObjSetNextAlloc(void)
{
	AObj *aobj;

	if (sGCAnimHead == NULL)
	{
		sGCAnimHead = syTaskmanMalloc(sizeof(AObj), 0x4);

		sGCAnimHead->next = NULL;
	}
	if (sGCAnimHead == NULL)
	{
		syDebugPrintf("om : couldn't get AObj\n");
		while (TRUE);
	}
	aobj = sGCAnimHead;
	sGCAnimHead = sGCAnimHead->next;
	sGCAnimsActiveNum++;

	return aobj;
}

void gcAppendAObjToDObj(DObj *dobj, AObj *aobj)
{
	aobj->next = dobj->aobj;
	dobj->aobj = aobj;
}

AObj* gcAddAObjForDObj(DObj *dobj, u8 track)
{
	AObj *aobj = gcGetAObjSetNextAlloc();

	aobj->track = track;
	aobj->kind = nGCAnimKindNone;
	aobj->interpolate = NULL;
	aobj->rate_target = 0.0F;
	aobj->rate_base = 0.0F;
	aobj->value_target = 0.0F;
	aobj->value_base = 0.0F;
	aobj->length = 0.0F;
	aobj->length_invert = 1.0F;

	gcAppendAObjToDObj(dobj, aobj);

	return aobj;
}

f32 ftAnimGetTargetValue(s16 arg, s32 track, sb32 value_or_step)
{
    f32 unused;
    s32 id;
    f32 ret;

    // 0x8012B910
    f32 fracs[/* */] =
    {
        1.0F / 512.0F,
        1.0F / 4.0F,
        1.0F / 4096.0F,
        1.0F / 16384.0F - (3.0F / 1000000000000.0F), // why tho
        1.0F / 512.0F,
        1.0F / 32.0F,
        1.0F / 8192.0F,
        1.0F / 16384.0F - (3.0F / 1000000000000.0F)  // ???
    };

    switch (track)
    {
    case nGCAnimTrackRotX:
    case nGCAnimTrackRotY:
    case nGCAnimTrackRotZ:
        id = 0;
        break;

    case nGCAnimTrackTraX:
    case nGCAnimTrackTraY:
    case nGCAnimTrackTraZ:
        id = 1;
        break;

    case nGCAnimTrackScaX:
    case nGCAnimTrackScaY:
    case nGCAnimTrackScaZ:
        id = 2;
        break;

    case nGCAnimTrackTraI:
        id = 3;
        break;
    }
    if (value_or_step != 0)
    {
        id += 4;
    }
    ret = arg;

    ret *= fracs[id];

    return ret;
}

void ftAnimParseDObjFigatree(DObj *root_dobj)
{
    AObj *track_aobjs[nGCAnimTrackJointEnd - nGCAnimTrackJointStart + 1];
    AObj *current_aobj;
    f32 payload;
    s32 i;
    u16 command_kind;
    u16 flags;

    if (root_dobj->anim_wait != AOBJ_ANIM_NULL)
    {
        if (root_dobj->anim_wait == AOBJ_ANIM_CHANGED)
        {
            root_dobj->anim_wait = -root_dobj->anim_frame;
        }
        else
        {
            root_dobj->anim_wait -= root_dobj->anim_speed;
            root_dobj->anim_frame += root_dobj->anim_speed;
            root_dobj->parent_gobj->anim_frame = root_dobj->anim_frame;

            if (root_dobj->anim_wait > 0.0F)
            {
                return;
            }
        }
        for (i = 0; i < ARRAY_COUNT(track_aobjs); i++)
        {
            track_aobjs[i] = NULL;
        }
        current_aobj = root_dobj->aobj;

        while (current_aobj != NULL)
        {
            if ((current_aobj->track >= nGCAnimTrackJointStart) && (current_aobj->track <= ARRAY_COUNT(track_aobjs)))
            {
                track_aobjs[current_aobj->track - nGCAnimTrackJointStart] = current_aobj;
            }
            current_aobj = current_aobj->next;
        }
        do
        {
            if (root_dobj->anim_joint.event16 == NULL)
            {
                current_aobj = root_dobj->aobj;

                while (current_aobj != NULL)
                {
                    if (current_aobj->kind != nGCAnimKindNone)
                    {
                        current_aobj->length += root_dobj->anim_speed + root_dobj->anim_wait;
                    }
                    current_aobj = current_aobj->next;
                }
                root_dobj->anim_frame = root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = root_dobj->anim_wait;
                root_dobj->anim_wait = AOBJ_ANIM_END;

                return;
            }
            command_kind = root_dobj->anim_joint.event16->command.opcode;

            switch (command_kind)
            {
            case nGCAnimEvent16SetVal0RateBlock:
            case nGCAnimEvent16SetVal0Rate:
                flags = root_dobj->anim_joint.event16->command.flags;
                payload = (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle) ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u : 0.0F;

                for (i = 0; i < ARRAY_COUNT(track_aobjs); i++, flags = flags >> 1)
                {
                    if (!(flags)) 
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        if (track_aobjs[i] == NULL)
                        {
                            track_aobjs[i] = gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);
                        }
                        track_aobjs[i]->value_base = track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = ftAnimGetTargetValue(AObjAnimAdvance(root_dobj->anim_joint.event16)->s, i + nGCAnimTrackJointStart, 0);

                        track_aobjs[i]->rate_base = track_aobjs[i]->rate_target;
                        track_aobjs[i]->rate_target = 0.0F;

                        track_aobjs[i]->kind = nGCAnimKindCubic;

                        if (payload != 0.0F)
                        {
                            track_aobjs[i]->length_invert = 1.0F / payload;
                        }
                        track_aobjs[i]->length = -root_dobj->anim_wait - root_dobj->anim_speed;
                    }
                }
                if (command_kind == nGCAnimEvent16SetVal0RateBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetValBlock:
            case nGCAnimEvent16SetVal:
                flags = root_dobj->anim_joint.event16->command.flags;
                payload = (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle) ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u : 0.0F;

                for (i = 0; i < ARRAY_COUNT(track_aobjs); i++, flags = flags >> 1)
                {
                    if (!(flags)) 
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        if (track_aobjs[i] == NULL)
                        {
                            track_aobjs[i] = gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);
                        }
                        track_aobjs[i]->value_base = track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = ftAnimGetTargetValue(AObjAnimAdvance(root_dobj->anim_joint.event16)->s, i + nGCAnimTrackJointStart, 0);

                        track_aobjs[i]->kind = nGCAnimKindLinear;

                        if (payload != 0.0F)
                        {
                            track_aobjs[i]->rate_base = (track_aobjs[i]->value_target - track_aobjs[i]->value_base) / payload;
                        }
                        track_aobjs[i]->length = -root_dobj->anim_wait - root_dobj->anim_speed;
                        track_aobjs[i]->rate_target = 0.0F;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetValRateBlock:
            case nGCAnimEvent16SetValRate:
                flags = root_dobj->anim_joint.event16->command.flags;
                payload = (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle) ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u : 0.0F;

                for (i = 0; i < ARRAY_COUNT(track_aobjs); i++, flags = flags >> 1)
                {
                    if (!(flags)) 
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        if (track_aobjs[i] == NULL)
                        {
                            track_aobjs[i] = gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);
                        }
                        track_aobjs[i]->value_base = track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = ftAnimGetTargetValue(AObjAnimAdvance(root_dobj->anim_joint.event16)->s, i + nGCAnimTrackJointStart, 0);

                        track_aobjs[i]->rate_base = track_aobjs[i]->rate_target;
                        track_aobjs[i]->rate_target = ftAnimGetTargetValue(AObjAnimAdvance(root_dobj->anim_joint.event16)->s, i + nGCAnimTrackJointStart, 1);

                        track_aobjs[i]->kind = nGCAnimKindCubic;

                        if (payload != 0.0F)
                        {
                            track_aobjs[i]->length_invert = 1.0F / payload;
                        }
                        track_aobjs[i]->length = -root_dobj->anim_wait - root_dobj->anim_speed;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValRateBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16SetTargetRate:
                flags = root_dobj->anim_joint.event16->command.flags;

                payload = (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle) ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u : 0.0F;

                for (i = 0; i < ARRAY_COUNT(track_aobjs); i++, flags = flags >> 1)
                {
                    if (!(flags)) 
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        if (track_aobjs[i] == NULL)
                        {
                            track_aobjs[i] = gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);
                        }
                        track_aobjs[i]->rate_target = ftAnimGetTargetValue(AObjAnimAdvance(root_dobj->anim_joint.event16)->s, i + nGCAnimTrackJointStart, 1);
                    }
                }
                break;

            case nGCAnimEvent16Block:
                if (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle)
                {
                    root_dobj->anim_wait += AObjAnimAdvance(root_dobj->anim_joint.event16)->u;
                }
                break;

            case nGCAnimEvent16SetValAfterBlock:
            case nGCAnimEvent16SetValAfter:
                flags = root_dobj->anim_joint.event16->command.flags;
                payload = (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle) ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u : 0.0F;

                for (i = 0; i < ARRAY_COUNT(track_aobjs); i++, flags = flags >> 1)
                {
                    if (!(flags)) 
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        if (track_aobjs[i] == NULL)
                        {
                            track_aobjs[i] = gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);
                        }
                        track_aobjs[i]->value_base = track_aobjs[i]->value_target;
                        track_aobjs[i]->value_target = ftAnimGetTargetValue(AObjAnimAdvance(root_dobj->anim_joint.event16)->s, i + nGCAnimTrackJointStart, 0);

                        track_aobjs[i]->kind = nGCAnimKindStep;

                        track_aobjs[i]->length_invert = payload;
                        track_aobjs[i]->length = -root_dobj->anim_wait - root_dobj->anim_speed;

                        track_aobjs[i]->rate_target = 0.0F;
                    }
                }
                if (command_kind == nGCAnimEvent16SetValAfterBlock)
                {
                    root_dobj->anim_wait += payload;
                }
                break;

            case nGCAnimEvent16Loop:
                AObjAnimAdvance(root_dobj->anim_joint.event16);

                root_dobj->anim_joint.event16 += root_dobj->anim_joint.event16->s / 2;

                root_dobj->anim_frame = -root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = -root_dobj->anim_wait;

                if (root_dobj->is_anim_root != FALSE)
                {
                    if (root_dobj->parent_gobj->func_anim != NULL)
                    {
                        root_dobj->parent_gobj->func_anim(root_dobj, -2, 0);
                    }
                }
                break;

            case nGCAnimEvent1611:
                flags = root_dobj->anim_joint.event16->command.flags;

                payload = (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle) ? AObjAnimAdvance(root_dobj->anim_joint.event16)->u : 0.0F;

                for (i = 0; i < ARRAY_COUNT(track_aobjs); i++, flags = flags >> 1)
                {
                    if (!(flags)) 
                    {
                        break;
                    }
                    if (flags & 1)
                    {
                        if (track_aobjs[i] == NULL)
                        {
                            track_aobjs[i] = gcAddAObjForDObj(root_dobj, i + nGCAnimTrackJointStart);
                        }
                        track_aobjs[i]->length += payload;
                    }
                }
                break;

            case nGCAnimEvent16SetTranslateInterp:
                AObjAnimAdvance(root_dobj->anim_joint.event16);

                if (track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] == NULL)
                {
                    track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart] = gcAddAObjForDObj(root_dobj, nGCAnimTrackTraI);
                }
                track_aobjs[nGCAnimTrackTraI - nGCAnimTrackJointStart]->interpolate = root_dobj->anim_joint.event16 + (root_dobj->anim_joint.event16->s / 2);

                AObjAnimAdvance(root_dobj->anim_joint.event16);
                break;

            case nGCAnimEvent32End:
                current_aobj = root_dobj->aobj;

                while (current_aobj != NULL)
                {
                    if (current_aobj->kind != nGCAnimKindNone)
                    {
                        current_aobj->length += root_dobj->anim_speed + root_dobj->anim_wait;
                    }
                    current_aobj = current_aobj->next;
                }
                root_dobj->anim_frame = root_dobj->anim_wait;
                root_dobj->parent_gobj->anim_frame = root_dobj->anim_wait;
                root_dobj->anim_wait = AOBJ_ANIM_END;

                if (root_dobj->is_anim_root != FALSE)
                {
                    if (root_dobj->parent_gobj->func_anim != NULL)
                    {
                        root_dobj->parent_gobj->func_anim(root_dobj, -1, 0);
                    }
                }
                return;

            case nGCAnimEvent16SetFlags:
                root_dobj->flags = root_dobj->anim_joint.event16->command.flags;

                if (AObjAnimAdvance(root_dobj->anim_joint.event16)->command.toggle)
                {
                    root_dobj->anim_wait += AObjAnimAdvance(root_dobj->anim_joint.event16)->u;
                }
                break;

            default:
                break;
            }
        } 
        while (root_dobj->anim_wait <= 0.0F);
    }
}

#define GOBJ_FLAG_NOANIM            (1 << 1)            // Skip applying aninmation values?
#define SQUARE(x) ((x) * (x))


void gcPlayDObjAnimJoint(DObj *dobj)
{
    f32 value; // array_dobjs
    AObj *aobj;
    f32 temp_f16;
    f32 temp_f12;
    f32 temp_f18;
    f32 temp_f14;
    f32 temp_f20;
    f32 temp_f22;
    f32 temp_f24;

    if (dobj->anim_wait != AOBJ_ANIM_NULL) 
    {
        aobj = dobj->aobj;

        while (aobj != NULL)
        {
            if (aobj->kind != nGCAnimKindNone)
            {
                if (dobj->anim_wait != AOBJ_ANIM_END) 
                { 
                    aobj->length += dobj->anim_speed; 
                }
                if (!(dobj->parent_gobj->flags & GOBJ_FLAG_NOANIM)) 
                {
                    switch (aobj->kind) 
                    {
                    case nGCAnimKindLinear:
                        value = aobj->value_base + (aobj->length * aobj->rate_base); 
                        break;
                        
                    case nGCAnimKindCubic:
                        temp_f16 = SQUARE(aobj->length_invert);
                        temp_f12 = SQUARE(aobj->length);
                        temp_f18 = aobj->length_invert * temp_f12;
                        temp_f14 = aobj->length * temp_f12 * temp_f16;
                        temp_f20 = 2.0F * temp_f14 * aobj->length_invert;
                        temp_f22 = 3.0F * temp_f12 * temp_f16;
                        temp_f24 = temp_f14 - temp_f18;

                        value = 
                            (aobj->value_base * ((temp_f20 - temp_f22) + 1.0F)) + 
                            (aobj->value_target * (temp_f22 - temp_f20)) + 
                            (aobj->rate_base * ((temp_f24 - temp_f18) + aobj->length)) + 
                            (aobj->rate_target * temp_f24);
                        break;
                        
                    case nGCAnimKindStep: 
                        value = (aobj->length_invert <= aobj->length) ? aobj->value_target : aobj->value_base; 
                        break;

                    default:
                        break;
                    }
                    switch (aobj->track)
                    {
                    case nGCAnimTrackRotX: 
                        dobj->rotate.vec.f.x = value;
                        break;

                    case nGCAnimTrackRotY: 
                        dobj->rotate.vec.f.y = value;
                        break;

                    case nGCAnimTrackRotZ: 
                        dobj->rotate.vec.f.z = value;
                        break;

                    case nGCAnimTrackTraI:
                        if (value < 0.0F) 
                        {
                            value = 0.0F;
                        } 
                        else if (value > 1.0F) 
                        {
                            value = 1.0F;
                        }
                        // syInterpCubic(&dobj->translate.vec.f, aobj->interpolate, value);
                        break;

                    case nGCAnimTrackTraX: 
                        dobj->translate.vec.f.x = value; 
                        break;

                    case nGCAnimTrackTraY: 
                        dobj->translate.vec.f.y = value; 
                        break;

                    case nGCAnimTrackTraZ: 
                        dobj->translate.vec.f.z = value; 
                        break;

                    case nGCAnimTrackScaX: 
                        dobj->scale.vec.f.x = value; 
                        break;
                        
                    case nGCAnimTrackScaY: 
                        dobj->scale.vec.f.y = value; 
                        break;

                    case nGCAnimTrackScaZ: 
                        dobj->scale.vec.f.z = value; 
                        break;
                    }
                }
            }
            aobj = aobj->next;
        }
        if (dobj->anim_wait == AOBJ_ANIM_END) 
        { 
            dobj->anim_wait = AOBJ_ANIM_NULL;
        }
    }
}

void ftParamUpdateAnimKeys(GObj* fighter_gobj) {
    FTStruct* fp = ftGetStruct(fighter_gobj);
    DObj** p_joint = &fp->joints[nFTPartsJointTopN];
    DObj* joint;
    MObj* mobj;
    FTParts* parts;
    f32 anim_wait_bak;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(fp->joints); i++, p_joint++)
    {
        joint = *p_joint;

        if (joint != NULL)
        {
            ftAnimParseDObjFigatree(joint);

           gcPlayDObjAnimJoint(joint);
        }
    }
}