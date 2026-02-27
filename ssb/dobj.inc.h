DObj* gcGetDObjSetNextAlloc(void)
{
    DObj* dobj;

    if (sGCDrawHead == NULL)
    {
        sGCDrawHead = syTaskmanMalloc(sGCDrawSize, 0x8);

        sGCDrawHead->alloc_free = NULL;
    }
    if (sGCDrawHead == NULL)
    {
        syDebugPrintf("om : couldn't get DObj\n");
        while (TRUE);
    }
    dobj = sGCDrawHead;
    sGCDrawHead = sGCDrawHead->alloc_free;
    sGCDrawsActiveNum++;

    return dobj;
}

void gcInitDObj(DObj* dobj)
{
    s32 i;

    dobj->vec = NULL;
    dobj->flags = DOBJ_FLAG_NONE;
    dobj->is_anim_root = FALSE;
    dobj->xobjs_num = 0;

    for (i = 0; i < ARRAY_COUNT(dobj->xobjs); i++)
    {
        dobj->xobjs[i] = NULL;
    }
    dobj->aobj = NULL;
    dobj->anim_joint.event32 = NULL;
    dobj->anim_wait = AOBJ_ANIM_NULL;
    dobj->anim_speed = 1.0F;
    dobj->anim_frame = 0.0F;
    dobj->mobj = NULL;
    dobj->user_data.p = NULL;
}

DObj* gcAddDObjForGObj(GObj* gobj, void* dvar)
{
    DObj* new_dobj, * current_dobj;

    new_dobj = gcGetDObjSetNextAlloc();

    if (gobj->obj != NULL)
    {
        current_dobj = DObjGetStruct(gobj);

        while (current_dobj->sib_next != NULL)
        {
            current_dobj = current_dobj->sib_next;
        }
        current_dobj->sib_next = new_dobj;
        new_dobj->sib_prev = current_dobj;
    }
    else
    {
        gobj->obj_kind = nGCCommonAppendDObj;
        gobj->obj = new_dobj;
        new_dobj->sib_prev = NULL;
    }
    new_dobj->parent_gobj = gobj;
    new_dobj->parent = DOBJ_PARENT_NULL;
    new_dobj->sib_next = NULL;
    new_dobj->child = NULL;
    new_dobj->dv = dvar;

    gcInitDObj(new_dobj);

    return new_dobj;
}

DObj* gcAddChildForDObj(DObj* dobj, void* dvar)
{
    DObj* new_dobj = gcGetDObjSetNextAlloc();
    DObj* current_dobj;

    if (dobj->child != NULL)
    {
        current_dobj = dobj->child;

        while (current_dobj->sib_next != NULL)
        {
            current_dobj = current_dobj->sib_next;
        }
        current_dobj->sib_next = new_dobj;
        new_dobj->sib_prev = current_dobj;
    }
    else
    {
        dobj->child = new_dobj;
        new_dobj->sib_prev = NULL;
    }
    new_dobj->parent_gobj = dobj->parent_gobj;
    new_dobj->parent = dobj;
    new_dobj->child = NULL;
    new_dobj->sib_next = NULL;
    new_dobj->dv = dvar;

    gcInitDObj(new_dobj);

    return new_dobj;
}

MObj* gcGetMObjSetNextAlloc(void)
{
    MObj* mobj;

    if (sGCMaterialHead == NULL)
    {
        sGCMaterialHead = syTaskmanMalloc(sizeof(MObj), 0x4);
        sGCMaterialHead->next = NULL;
    }

    if (sGCMaterialHead == NULL)
    {
        syDebugPrintf("om : couldn't get MObj\n");
        while (TRUE);
    }

    mobj = sGCMaterialHead;
    sGCMaterialHead = sGCMaterialHead->next;
    sGCMaterialsActive++;

    return mobj;
}

MObj* gcAddMObjForDObj(DObj* dobj, MObjSub* mobjsub)
{
    MObj* mobj = gcGetMObjSetNextAlloc();

    if (dobj->mobj != NULL)
    {
        MObj* current_mobj = dobj->mobj->next;
        MObj* prior_mobj = dobj->mobj;

        while (current_mobj != NULL)
        {
            prior_mobj = current_mobj;
            current_mobj = current_mobj->next;
        }
        prior_mobj->next = mobj;
    }
    else dobj->mobj = mobj;

    mobj->next = NULL;
    mobj->lfrac = mobjsub->prim_l / 255.0F;
    mobj->sub = *mobjsub;

    mobj->sub.unk24 = mobjsub->trau;
    mobj->sub.unk28 = mobjsub->scau;
    mobj->texture_id_curr = 0;
    mobj->texture_id_next = 0;
    mobj->palette_id = 0;
    mobj->aobj = NULL;
    mobj->matanim_joint.event32 = NULL;
    mobj->anim_wait = AOBJ_ANIM_NULL;
    mobj->anim_speed = 1.0F;
    mobj->anim_frame = 0.0F;

    return mobj;
}

void lbCommonAddMObjForFighterPartsDObj
(
    DObj* dobj,
    MObjSub** mobjsubs,
    AObjEvent32** costume_matanim_joints,
    AObjEvent32** main_matanim_joints,
    f32 anim_frame
) {
    if (mobjsubs != NULL) {
        MObjSub* mobjsub = *mobjsubs;
        while (mobjsub != NULL) {
            MObj* mobj = gcAddMObjForDObj(dobj, mobjsub);
            mobjsubs++;
            mobjsub = *mobjsubs;
        }
    }
}

void lbCommonInitDObj(DObj* dobj, u8 tk1, u8 tk2, u8 tk3, u8 arg4)
{
    if (tk1 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk1, arg4);
    }
    if (tk2 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk2, arg4);
    }
    if (tk3 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk3, arg4);
    }
    dobj->translate.vec = dGCTranslateDefault.vec;
    dobj->rotate.vec = dGCRotateDefaultRpy.vec;
    dobj->scale.vec = dGCScaleDefault.vec;
}

void lbCommonSetupFighterPartsDObjs
(
    DObj* root_dobj,
    FTCommonPartContainer* commonparts_container,
    s32 detail_curr,
    DObj** dobjs,
    u32* setup_parts,
    u8 tk1,
    u8 tk2,
    u8 tk3,
    f32 anim_frame,
    u8 arg9
)
{
    s32 id;
    s32 i;
    u32 flags0;
    u32 flags1;
    u32 current_flags;
    DObj* array_dobjs[DOBJ_ARRAY_MAX];
    s32 detail_id;
    DObjDesc* dobjdesc;
    DObj* current_dobj;

    dobjdesc = commonparts_container->commonparts[detail_curr - 1].dobjdesc;

    flags0 = setup_parts[0], flags1 = setup_parts[1];

    for (i = 0; i < ARRAY_COUNT(array_dobjs); i++)
    {
        array_dobjs[i] = NULL;
    }
    for (i = 0; ((flags0 != 0) || (flags1 != 0)) && (dobjdesc->id != ARRAY_COUNT(array_dobjs)); i++)
    {
        current_flags = (i < NBITS(u32)) ? flags0 : flags1;

        if (current_flags & (1 << 31))
        {
            id = dobjdesc->id & 0xFFF;

            if
                (
                    (detail_curr == nFTPartsDetailHigh) ||
                    (commonparts_container->commonparts[nFTPartsDetailLow - nFTPartsDetailStart].dobjdesc[i].dl == NULL)
                    )
            {
                detail_id = 0;
            }
            else detail_id = 1;

            if (id != 0)
            {
                current_dobj = array_dobjs[id] = gcAddChildForDObj
                (
                    array_dobjs[id - 1],
                    commonparts_container->commonparts[detail_id].dobjdesc[i].dl
                );
            }
            else {
                current_dobj = array_dobjs[0] = gcAddChildForDObj
                (
                    root_dobj,
                    commonparts_container->commonparts[detail_id].dobjdesc[i].dl
                );
            }

            if (dobjdesc->id & 0x8000)
            {
                //　こっちこない
                // gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3, 0x8000);
            }
            else {
                // 26個ともこっち
                lbCommonInitDObj(current_dobj, tk1, tk2, tk3, arg9);
            }

            current_dobj->translate.vec.f = dobjdesc->translate;
            current_dobj->rotate.vec.f = dobjdesc->rotate;
            current_dobj->scale.vec.f = dobjdesc->scale;

            lbCommonAddMObjForFighterPartsDObj
            (
                current_dobj,
                (commonparts_container->commonparts[detail_id].p_mobjsubs != NULL) ?
                commonparts_container->commonparts[detail_id].p_mobjsubs[i] : NULL,
                (commonparts_container->commonparts[detail_id].p_costume_matanim_joints != NULL) ?
                commonparts_container->commonparts[detail_id].p_costume_matanim_joints[i] : NULL,
                NULL,
                anim_frame
            );

            if (dobjs != NULL)
            {
                *dobjs = current_dobj;
            }
        }
        dobjs++;
        dobjdesc++;

        if (i < NBITS(u32))
        {
            flags0 <<= 1;
        }
        else flags1 <<= 1;
    }
}

XObj* gcGetXObjSetNextAlloc(void)
{
    XObj* xobj;

    if (sGCMatrixHead == NULL)
    {
        sGCMatrixHead = syTaskmanMalloc(sizeof(XObj), 0x8);
        sGCMatrixHead->next = NULL;
    }
    if (sGCMatrixHead == NULL)
    {
        syDebugPrintf("om : couldn't get OMMtx\n");
        while (TRUE);
    }
    xobj = sGCMatrixHead;
    sGCMatrixHead = sGCMatrixHead->next;
    sGCMatrixesActiveNum++;

    return xobj;
}

XObj* gcAddXObjForDObjVar(DObj* dobj, u8 kind, u8 arg2, s32 xobj_id)
{
    uintptr_t csr;
    GCTranslate* translate;
    GCRotate* rotate;
    GCScale* scale;
    XObj* xobj;
    s32 i;

    if (dobj->xobjs_num == ARRAY_COUNT(dobj->xobjs))
    {
        syDebugPrintf("om : couldn\'t add OMMtx for DObj\n");
        while (TRUE);
    }

    for (i = dobj->xobjs_num; i > xobj_id; i--)
    {
        dobj->xobjs[i] = dobj->xobjs[i - 1];
    }
    dobj->xobjs_num++;

    dobj->xobjs[xobj_id] = xobj = gcGetXObjSetNextAlloc();
    xobj->kind = kind;

    xobj->unk05 = arg2;

    return xobj;
}

XObj* gcAddXObjForDObjFixed(DObj* dobj, u8 kind, u8 arg2)
{
    return gcAddXObjForDObjVar(dobj, kind, arg2, dobj->xobjs_num);
}

void gcAddDObj3TransformsKind(DObj* dobj, u8 tk1, u8 tk2, u8 tk3)
{
    if (tk1 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk1, 0);
    }
    if (tk2 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk2, 0);
    }
    if (tk3 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk3, 0);
    }
}

void lbCommonInitDObj3Transforms(DObj* dobj, u8 tk1, u8 tk2, u8 tk3)
{
    gcAddDObj3TransformsKind(dobj, tk1, tk2, tk3);
}



void gcDrawMObjForDObj(DObj* dobj, Gfx** dl_head) {
    s32 mobj_count;
    s32 i;
    MObj* mobj;
    Gfx* new_dl;
    Gfx* branch_dl;
    u32 flags;
    f32 scau;
    f32 scav;
    f32 trau;
    f32 trav;
    f32 scrollu;
    f32 scrollv;
    s32 uls, ult;
    s32 s, t;

    if (dobj->mobj == NULL)
    {
        return;
    }
    gSPSegment((*dl_head)++, 0xE, gSYTaskmanGraphicsHeap.ptr);
    for (mobj_count = 0, mobj = dobj->mobj; mobj != NULL; mobj_count++)
    {
        mobj = mobj->next;
    }
    mobj = dobj->mobj;
    branch_dl = (Gfx*)(((Gfx*)gSYTaskmanGraphicsHeap.ptr) + mobj_count);
    new_dl = gSYTaskmanGraphicsHeap.ptr;

    for (int i = 0; i < mobj_count; i++, mobj = mobj->next) {

        flags = mobj->sub.flags;

        if (flags == MOBJ_FLAG_NONE)
        {
            flags = (MOBJ_FLAG_TEXTURE | 0x20 | MOBJ_FLAG_ALPHA);
        }
        if (flags & (MOBJ_FLAG_TEXTURE | 0x40 | 0x20))
        {
            scau = mobj->sub.scau;
            scav = mobj->sub.scav;
            trau = mobj->sub.trau;
            trav = mobj->sub.trav;
            scrollu = mobj->sub.scrollu;
            scrollv = mobj->sub.scrollv;

            if (mobj->sub.unk10 == 1)
            {
                scau *= 0.5F;
                trau = ((trau - mobj->sub.unk24) + 1.0F - (mobj->sub.unk28 * 0.5F)) * 0.5F;
                scrollu = ((scrollu - mobj->sub.unk44) + 1.0F - (mobj->sub.unk28 * 0.5F)) * 0.5F;
            }
        }

        gSPBranchList(&new_dl[i], branch_dl);

        if (flags & MOBJ_FLAG_PALETTE)
        {
            gDPSetTextureImage(branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, mobj->sub.palettes[(s32)mobj->palette_id]);

            if (flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA))
            {
                gDPTileSync(branch_dl++);
                gDPSetTile
                (
                    branch_dl++,
                    G_IM_FMT_RGBA,
                    G_IM_SIZ_4b,
                    0,
                    0x0100,
                    5,
                    0,
                    G_TX_NOMIRROR | G_TX_WRAP,
                    G_TX_NOMASK,
                    G_TX_NOLOD,
                    G_TX_NOMIRROR | G_TX_WRAP,
                    G_TX_NOMASK,
                    G_TX_NOLOD
                );
                gDPLoadSync(branch_dl++);
                gDPLoadTLUTCmd(branch_dl++, 5, (mobj->sub.siz == G_IM_SIZ_8b) ? 0xFF : 0xF);
                gDPPipeSync(branch_dl++);
            }
        }
        if (flags & MOBJ_FLAG_LIGHT1)
        {
            gSPLightColor(branch_dl++, LIGHT_1, mobj->sub.light1color.pack);
        }
        if (flags & MOBJ_FLAG_LIGHT2)
        {
            gSPLightColor(branch_dl++, LIGHT_2, mobj->sub.light2color.pack);
        }
        if (flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8))
        {
            if (flags & MOBJ_FLAG_FRAC)
            {
                s32 trunc = mobj->lfrac;

                gDPSetPrimColor
                (
                    branch_dl++,
                    mobj->sub.prim_m,
                    (mobj->lfrac - trunc) * 256.0F,
                    mobj->sub.primcolor.s.r,
                    mobj->sub.primcolor.s.g,
                    mobj->sub.primcolor.s.b,
                    mobj->sub.primcolor.s.a
                );
                mobj->texture_id_curr = trunc;
                mobj->texture_id_next = trunc + 1;
            }
            else
            {
                gDPSetPrimColor
                (
                    branch_dl++,
                    mobj->sub.prim_m,
                    mobj->lfrac * 255.0F,
                    mobj->sub.primcolor.s.r,
                    mobj->sub.primcolor.s.g,
                    mobj->sub.primcolor.s.b,
                    mobj->sub.primcolor.s.a
                );
            }
        }
        if (flags & MOBJ_FLAG_ENVCOLOR)
        {
            gDPSetEnvColor
            (
                branch_dl++,
                mobj->sub.envcolor.s.r,
                mobj->sub.envcolor.s.g,
                mobj->sub.envcolor.s.b,
                mobj->sub.envcolor.s.a
            );
        }
        if (flags & MOBJ_FLAG_BLENDCOLOR)
        {
            gDPSetBlendColor
            (
                branch_dl++,
                mobj->sub.blendcolor.s.r,
                mobj->sub.blendcolor.s.g,
                mobj->sub.blendcolor.s.b,
                mobj->sub.blendcolor.s.a
            );
        }
        if (flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT))
        {
            s32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ? G_IM_SIZ_32b : G_IM_SIZ_16b;

            gDPSetTextureImage
            (
                branch_dl++,
                mobj->sub.block_fmt,
                block_siz,
                1,
                mobj->sub.sprites[mobj->texture_id_next]
            );
            if (flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA))
            {
                gDPLoadSync(branch_dl++);

                switch (mobj->sub.block_siz)
                {
                case G_IM_SIZ_4b:
                    gDPLoadBlock
                    (
                        branch_dl++,
                        6,
                        0,
                        0,
                        (((mobj->sub.block_dxt * mobj->sub.unk36) + 3) >> 2) - 1,
                        (((mobj->sub.block_dxt / 16 <= 0) ? 1 : mobj->sub.block_dxt / 16) + 0x7FF) / ((mobj->sub.block_dxt / 16 <= 0) ? 1 : mobj->sub.block_dxt / 16)
                    );
                    break;

                case G_IM_SIZ_8b:
                    gDPLoadBlock
                    (
                        branch_dl++,
                        6,
                        0,
                        0,
                        (((mobj->sub.block_dxt * mobj->sub.unk36) + 1) >> 1) - 1,
                        (((mobj->sub.block_dxt / 8 <= 0) ? 1 : mobj->sub.block_dxt / 8) + 0x7FF) / ((mobj->sub.block_dxt / 8 <= 0) ? 1 : mobj->sub.block_dxt / 8)
                    );
                    break;

                case G_IM_SIZ_16b:
                    gDPLoadBlock
                    (
                        branch_dl++,
                        6,
                        0,
                        0,
                        (mobj->sub.block_dxt * mobj->sub.unk36) - 1,
                        ((((mobj->sub.block_dxt * 2) / 8 <= 0) ? 1 : (mobj->sub.block_dxt * 2) / 8) + 0x7FF) / (((mobj->sub.block_dxt * 2) / 8 <= 0) ? 1 : (mobj->sub.block_dxt * 2) / 8)
                    );
                    break;

                case G_IM_SIZ_32b:
                    gDPLoadBlock
                    (
                        branch_dl++,
                        6,
                        0,
                        0,
                        (mobj->sub.block_dxt * mobj->sub.unk36) - 1,
                        ((((mobj->sub.block_dxt * 4) / 8 <= 0) ? 1 : (mobj->sub.block_dxt * 4) / 8) + 0x7FF) / (((mobj->sub.block_dxt * 4) / 8 <= 0) ? 1 : (mobj->sub.block_dxt * 4) / 8)
                    );
                    break;
                }
                gDPLoadSync(branch_dl++);
            }
        }
        if (flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA))
        {
            gDPSetTextureImage
            (
                branch_dl++,
                mobj->sub.fmt,
                mobj->sub.siz,
                1,
                mobj->sub.sprites[mobj->texture_id_curr]
            );
        }
        if (flags & 0x20)
        {
            if (mobj->sub.unk10 == 2)
            {
                uls = (ABSF(scau) > (1.0F / 65535.0F)) ? ((mobj->sub.unk0C * trau) / scau) * 4.0F : 0.0F;
                ult = (ABSF(scav) > (1.0F / 65535.0F)) ? ((mobj->sub.unk0E * trav) / scav) * 4.0F : 0.0F;

                if (uls < 0)
                {
                    uls = 0;
                }
                if (ult < 0)
                {
                    ult = 0;
                }
            }
            else
            {
                uls = (ABSF(scau) > (1.0F / 65535.0F)) ? (((mobj->sub.unk0C * trau) + mobj->sub.unk0A) / scau) * 4.0F : 0.0F;
                ult = (ABSF(scav) > (1.0F / 65535.0F)) ? (((((1.0F - scav) - trav) * mobj->sub.unk0E) + mobj->sub.unk0A) / scav) * 4.0F : 0.0F;
            }
            gDPSetTileSize
            (
                branch_dl++,
                0,
                uls,
                ult,
                ((mobj->sub.unk0C - 1) << 2) + uls,
                ((mobj->sub.unk0E - 1) << 2) + ult
            );
        }
        if (flags & 0x40)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ? (((mobj->sub.unk38 * scrollu) + mobj->sub.unk0A) / scau) * 4.0F : 0.0F;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ? (((((1.0F - scav) - scrollv) * mobj->sub.unk3A) + mobj->sub.unk0A) / scav) * 4.0F : 0.0F;

            gDPSetTileSize
            (
                branch_dl++,
                1,
                uls,
                ult,
                ((mobj->sub.unk38 - 1) << 2) + uls,
                ((mobj->sub.unk3A - 1) << 2) + ult
            );
        }
        if (flags & MOBJ_FLAG_TEXTURE)
        {
            if (mobj->sub.unk10 == 2)
            {
                s = (ABSF(scau) > (1.0F / 65535.0F)) ? (mobj->sub.unk0C * 64) / scau : 0.0F;
                t = (ABSF(scav) > (1.0F / 65535.0F)) ? (mobj->sub.unk0E * 64) / scav : 0.0F;
            }
            else
            {
                s = (ABSF(scau) > (1.0F / 65535.0F)) ? (2097152.0F / mobj->sub.unk08) / scau : 0.0F;
                t = (ABSF(scav) > (1.0F / 65535.0F)) ? (2097152.0F / mobj->sub.unk08) / scav : 0.0F;
            }
            if (s > 0xFFFF)
            {
                s = 0xFFFF;
            }
            if (t > 0xFFFF)
            {
                t = 0xFFFF;
            }
            gSPTexture(branch_dl++, s, t, 0, 0, G_ON);
        }

        gSPEndDisplayList(branch_dl++);
    }
    gSYTaskmanGraphicsHeap.ptr = (void*)branch_dl;
}

void syMatrixTraRotRpyR(Mtx* m, f32 tx, f32 ty, f32 tz, f32 r, f32 p, f32 y)
{
    s32 sinr, sinp, siny;
    s32 cosr, cosp, cosy;
    u16 indexr, indexp, indexy;
    u32 e1, e2;

    syGetSinCosUShort(sinr, cosr, r, indexr);
    syGetSinCosUShort(sinp, cosp, p, indexp);
    syGetSinCosUShort(siny, cosy, y, indexy);

    // [0, 0] -> [0, 3]
    e1 = (cosp * cosy) >> 14;
    e2 = (cosp * siny) >> 14;
    m->m[0][0] = COMBINE_INTEGRAL(e1, e2);
    m->m[2][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = -sinp * 2;
    m->m[0][1] = COMBINE_INTEGRAL(e1, FTOFIX32(0.0F));
    m->m[2][1] = COMBINE_FRACTIONAL(e1, FTOFIX32(0.0F));

    // [1, 0] -> [1, 3]
    e1 = ((((sinr * sinp) >> 15) * cosy) >> 14) - ((cosr * siny) >> 14);
    e2 = ((((sinr * sinp) >> 15) * siny) >> 14) + ((cosr * cosy) >> 14);
    m->m[0][2] = COMBINE_INTEGRAL(e1, e2);
    m->m[2][2] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (sinr * cosp) >> 14;
    m->m[0][3] = COMBINE_INTEGRAL(e1, FTOFIX32(0.0F));
    m->m[2][3] = COMBINE_FRACTIONAL(e1, FTOFIX32(0.0F));

    // [2, 0] -> [2, 3]
    e1 = ((((cosr * sinp) >> 15) * cosy) >> 14) + ((sinr * siny) >> 14);
    e2 = ((((cosr * sinp) >> 15) * siny) >> 14) - ((sinr * cosy) >> 14);

    m->m[1][0] = COMBINE_INTEGRAL(e1, e2);
    m->m[3][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (cosr * cosp) >> 14;
    m->m[1][1] = COMBINE_INTEGRAL(e1, FTOFIX32(0.0F));
    m->m[3][1] = COMBINE_FRACTIONAL(e1, FTOFIX32(0.0F));

    // [3, 0] -> [3, 3]
    // [3, 0] -> [3, 3]
    e1 = FTOFIX32(tx);
    e2 = FTOFIX32(ty);
    m->m[1][2] = COMBINE_INTEGRAL(e1, e2);
    m->m[3][2] = COMBINE_FRACTIONAL(e1, e2);

    e1 = FTOFIX32(tz);
    m->m[1][3] = COMBINE_INTEGRAL(e1, FTOFIX32(1.0F));
    m->m[3][3] = COMBINE_FRACTIONAL(e1, FTOFIX32(1.0F));
}

sb32 lbCommonFighterPartsFuncMatrix(Mtx* mtx, DObj* dobj, Gfx** dls)
{
    syMatrixTraRotRpyR
    (
        mtx,
        dobj->translate.vec.f.x,
        dobj->translate.vec.f.y,
        dobj->translate.vec.f.z,
        dobj->rotate.vec.f.x,
        dobj->rotate.vec.f.y,
        dobj->rotate.vec.f.z
    );

    return 0;
}

s32 gcPrepDObjMatrix(Gfx** dl, DObj* dobj) {
    Gfx* current_dl = *dl;
    s32 sp2CC;
    XObj* xobj;
    s32 ret;
    SYMatrixHub mtx_hub;
    s32 i;

    sp2CC = 0;

    for (i = 0; i < dobj->xobjs_num; i++) {
        xobj = dobj->xobjs[i]; // s3
        if (xobj != NULL) {
            mtx_hub.gbi = &xobj->mtx;

            if (xobj->unk05 != 2) {
                ret = 0;

                switch (xobj->kind)
                {
                default:
                    if (xobj->kind >= 66)
                    {
                        ret = lbCommonFighterPartsFuncMatrix(mtx_hub.gbi, dobj, &current_dl);
                    }
                    if (ret == 1)
                    {
                        continue;
                    }
                    else break;
                }
            check_05:
                if (xobj->unk05 == 1)
                {
                    if (mtx_hub.gbi == &xobj->mtx)
                    {
                        xobj->unk05 = 2;
                    }
                }
            }
            if (xobj->kind != 2)
            {
                if ((sp2CC == 0) && (dobj->parent == DOBJ_PARENT_NULL || dobj->sib_next != NULL))
                {
                    gSPMatrix(current_dl++, mtx_hub.gbi, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
                }
                else gSPMatrix(current_dl++, mtx_hub.gbi, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);

                sp2CC++;
            }
        }
    }

    *dl = current_dl;
    return sp2CC;
}

void ftDisplayMainDrawDefault(DObj* dobj) {
    DObj* sibling_dobj;
    s32 sp58;

    sp58 = gcPrepDObjMatrix(&gDisplayListHead, dobj); // AIが外に出してうまく行った

    if (dobj->dl != 0) {
        gcDrawMObjForDObj(dobj, &gDisplayListHead);

        gSPDisplayList(gDisplayListHead++, dobj->dl);
    }

    if (dobj->child != NULL)
    {
        ftDisplayMainDrawDefault(dobj->child);
    }

    if ((sp58 != FALSE) && ((dobj->parent == DOBJ_PARENT_NULL) || (dobj->sib_next != NULL)))
    {
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    }

    if (dobj->sib_prev == NULL)
    {
        sibling_dobj = dobj->sib_next;

        while (sibling_dobj != NULL)
        {
            ftDisplayMainDrawDefault(sibling_dobj);

            sibling_dobj = sibling_dobj->sib_next;
        }
    }
}