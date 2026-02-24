#define lbRelocGetFileData(type, file, offset) \
((type) ((uintptr_t)(file) + (intptr_t)(offset)))
XObj* gcAddXObjForDObjFixed(DObj* dobj, u8 kind, u8 arg2);

GObj gobj;
FTStruct fighter;
DObj* sGCDrawHead;
u32 sGCDrawsActiveNum;
u16 sGCDrawSize = sizeof(DObj);
MObj* sGCMaterialHead;
u32 sGCMaterialsActive;
SYMallocRegion gSYTaskmanGraphicsHeap;
SYMallocRegion sSYTaskmanDefaultGraphicsHeap[2];
s32 gSYTaskmanTaskID = 1;
XObj* sGCMatrixHead;
u32 sGCMatrixesActiveNum;
GCTranslate dGCTranslateDefault = { NULL, { 0.0F, 0.0F, 0.0F } };
GCRotate dGCRotateDefaultAXYZ = { NULL, 0.0F, { 0.0F, 0.0F, 1.0F } };
GCRotate dGCRotateDefaultRpy = { NULL, 0.0F, { 0.0F, 0.0F, 0.0F } };
GCScale dGCScaleDefault = { NULL, { 1.0F, 1.0F, 1.0F } };

typedef f32 Mat4[4][4];
typedef f32 Vec3fArray[3]; // X, Y, Z, where Y is up

#define DOBJ_PARENT_NULL            ((DObj*)1)
#define DOBJ_FLAG_NONE              (0)
#define DOBJ_FLAG_HIDDEN            (1 << 1)
#define F32_MAX 3.40282346639e+38F
#define F32_MIN (-F32_MAX)
#define AOBJ_ANIM_NULL              F32_MIN      
#define DObjGetStruct(gobj) ((DObj*)(gobj)->obj)
#define DOBJ_ARRAY_MAX              18
#define NBITS(t) ((int) (sizeof(t) * 8) )

// MObj defines - some of these are wild guesses...
#define MOBJ_FLAG_NONE              (0)
#define MOBJ_FLAG_ALPHA             (1 << 0)
#define MOBJ_FLAG_SPLIT             (1 << 1)    // Uses texture_id_next as texture image...?
#define MOBJ_FLAG_PALETTE           (1 << 2)
#define MOBJ_FLAG_FRAC              (1 << 4)
#define MOBJ_FLAG_TEXTURE           (1 << 7)
#define MOBJ_FLAG_PRIMCOLOR         (1 << 9)
#define MOBJ_FLAG_ENVCOLOR          (1 << 10)
#define MOBJ_FLAG_BLENDCOLOR        (1 << 11)
#define MOBJ_FLAG_LIGHT1            (1 << 12)
#define MOBJ_FLAG_LIGHT2            (1 << 13)

#define ABSF(x) ((x) < 0.0F ? -(x) : (x))

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

typedef union SYMatrixHub
{
    Mtx* gbi;
    Mtx44f* f;
    void** p;

} SYMatrixHub;

u16 gSYSinTable[/* */] =
{
    0x0000, 0x0032, 0x0064, 0x0096, 0x00C9, 0x00FB, 0x012D, 0x015F, 0x0192, 0x01C4, 0x01F6, 0x0228,
    0x025B, 0x028D, 0x02BF, 0x02F1, 0x0324, 0x0356, 0x0388, 0x03BA, 0x03ED, 0x041F, 0x0451, 0x0483,
    0x04B6, 0x04E8, 0x051A, 0x054C, 0x057F, 0x05B1, 0x05E3, 0x0615, 0x0647, 0x067A, 0x06AC, 0x06DE,
    0x0710, 0x0742, 0x0775, 0x07A7, 0x07D9, 0x080B, 0x083D, 0x086F, 0x08A2, 0x08D4, 0x0906, 0x0938,
    0x096A, 0x099C, 0x09CE, 0x0A00, 0x0A33, 0x0A65, 0x0A97, 0x0AC9, 0x0AFB, 0x0B2D, 0x0B5F, 0x0B91,
    0x0BC3, 0x0BF5, 0x0C27, 0x0C59, 0x0C8B, 0x0CBD, 0x0CEF, 0x0D21, 0x0D53, 0x0D85, 0x0DB7, 0x0DE9,
    0x0E1B, 0x0E4D, 0x0E7F, 0x0EB1, 0x0EE3, 0x0F15, 0x0F47, 0x0F79, 0x0FAB, 0x0FDD, 0x100E, 0x1040,
    0x1072, 0x10A4, 0x10D6, 0x1108, 0x1139, 0x116B, 0x119D, 0x11CF, 0x1201, 0x1232, 0x1264, 0x1296,
    0x12C8, 0x12F9, 0x132B, 0x135D, 0x138E, 0x13C0, 0x13F2, 0x1423, 0x1455, 0x1487, 0x14B8, 0x14EA,
    0x151B, 0x154D, 0x157F, 0x15B0, 0x15E2, 0x1613, 0x1645, 0x1676, 0x16A8, 0x16D9, 0x170A, 0x173C,
    0x176D, 0x179F, 0x17D0, 0x1802, 0x1833, 0x1864, 0x1896, 0x18C7, 0x18F8, 0x192A, 0x195B, 0x198C,
    0x19BD, 0x19EF, 0x1A20, 0x1A51, 0x1A82, 0x1AB3, 0x1AE4, 0x1B16, 0x1B47, 0x1B78, 0x1BA9, 0x1BDA,
    0x1C0B, 0x1C3C, 0x1C6D, 0x1C9E, 0x1CCF, 0x1D00, 0x1D31, 0x1D62, 0x1D93, 0x1DC4, 0x1DF5, 0x1E25,
    0x1E56, 0x1E87, 0x1EB8, 0x1EE9, 0x1F19, 0x1F4A, 0x1F7B, 0x1FAC, 0x1FDC, 0x200D, 0x203E, 0x206E,
    0x209F, 0x20D0, 0x2100, 0x2131, 0x2161, 0x2192, 0x21C2, 0x21F3, 0x2223, 0x2254, 0x2284, 0x22B4,
    0x22E5, 0x2315, 0x2345, 0x2376, 0x23A6, 0x23D6, 0x2407, 0x2437, 0x2467, 0x2497, 0x24C7, 0x24F7,
    0x2528, 0x2558, 0x2588, 0x25B8, 0x25E8, 0x2618, 0x2648, 0x2678, 0x26A8, 0x26D8, 0x2707, 0x2737,
    0x2767, 0x2797, 0x27C7, 0x27F6, 0x2826, 0x2856, 0x2886, 0x28B5, 0x28E5, 0x2915, 0x2944, 0x2974,
    0x29A3, 0x29D3, 0x2A02, 0x2A32, 0x2A61, 0x2A91, 0x2AC0, 0x2AEF, 0x2B1F, 0x2B4E, 0x2B7D, 0x2BAD,
    0x2BDC, 0x2C0B, 0x2C3A, 0x2C69, 0x2C98, 0x2CC8, 0x2CF7, 0x2D26, 0x2D55, 0x2D84, 0x2DB3, 0x2DE2,
    0x2E11, 0x2E3F, 0x2E6E, 0x2E9D, 0x2ECC, 0x2EFB, 0x2F29, 0x2F58, 0x2F87, 0x2FB5, 0x2FE4, 0x3013,
    0x3041, 0x3070, 0x309E, 0x30CD, 0x30FB, 0x312A, 0x3158, 0x3186, 0x31B5, 0x31E3, 0x3211, 0x3240,
    0x326E, 0x329C, 0x32CA, 0x32F8, 0x3326, 0x3354, 0x3382, 0x33B0, 0x33DE, 0x340C, 0x343A, 0x3468,
    0x3496, 0x34C4, 0x34F2, 0x351F, 0x354D, 0x357B, 0x35A8, 0x35D6, 0x3604, 0x3631, 0x365F, 0x368C,
    0x36BA, 0x36E7, 0x3714, 0x3742, 0x376F, 0x379C, 0x37CA, 0x37F7, 0x3824, 0x3851, 0x387E, 0x38AB,
    0x38D8, 0x3906, 0x3932, 0x395F, 0x398C, 0x39B9, 0x39E6, 0x3A13, 0x3A40, 0x3A6C, 0x3A99, 0x3AC6,
    0x3AF2, 0x3B1F, 0x3B4C, 0x3B78, 0x3BA5, 0x3BD1, 0x3BFD, 0x3C2A, 0x3C56, 0x3C83, 0x3CAF, 0x3CDB,
    0x3D07, 0x3D33, 0x3D60, 0x3D8C, 0x3DB8, 0x3DE4, 0x3E10, 0x3E3C, 0x3E68, 0x3E93, 0x3EBF, 0x3EEB,
    0x3F17, 0x3F43, 0x3F6E, 0x3F9A, 0x3FC5, 0x3FF1, 0x401D, 0x4048, 0x4073, 0x409F, 0x40CA, 0x40F6,
    0x4121, 0x414C, 0x4177, 0x41A2, 0x41CE, 0x41F9, 0x4224, 0x424F, 0x427A, 0x42A5, 0x42D0, 0x42FA,
    0x4325, 0x4350, 0x437B, 0x43A5, 0x43D0, 0x43FB, 0x4425, 0x4450, 0x447A, 0x44A5, 0x44CF, 0x44FA,
    0x4524, 0x454E, 0x4578, 0x45A3, 0x45CD, 0x45F7, 0x4621, 0x464B, 0x4675, 0x469F, 0x46C9, 0x46F3,
    0x471C, 0x4746, 0x4770, 0x479A, 0x47C3, 0x47ED, 0x4816, 0x4840, 0x4869, 0x4893, 0x48BC, 0x48E6,
    0x490F, 0x4938, 0x4961, 0x498A, 0x49B4, 0x49DD, 0x4A06, 0x4A2F, 0x4A58, 0x4A81, 0x4AA9, 0x4AD2,
    0x4AFB, 0x4B24, 0x4B4C, 0x4B75, 0x4B9E, 0x4BC6, 0x4BEF, 0x4C17, 0x4C3F, 0x4C68, 0x4C90, 0x4CB8,
    0x4CE1, 0x4D09, 0x4D31, 0x4D59, 0x4D81, 0x4DA9, 0x4DD1, 0x4DF9, 0x4E21, 0x4E48, 0x4E70, 0x4E98,
    0x4EBF, 0x4EE7, 0x4F0F, 0x4F36, 0x4F5E, 0x4F85, 0x4FAC, 0x4FD4, 0x4FFB, 0x5022, 0x5049, 0x5070,
    0x5097, 0x50BF, 0x50E5, 0x510C, 0x5133, 0x515A, 0x5181, 0x51A8, 0x51CE, 0x51F5, 0x521C, 0x5242,
    0x5269, 0x528F, 0x52B5, 0x52DC, 0x5302, 0x5328, 0x534E, 0x5375, 0x539B, 0x53C1, 0x53E7, 0x540D,
    0x5433, 0x5458, 0x547E, 0x54A4, 0x54CA, 0x54EF, 0x5515, 0x553A, 0x5560, 0x5585, 0x55AB, 0x55D0,
    0x55F5, 0x561A, 0x5640, 0x5665, 0x568A, 0x56AF, 0x56D4, 0x56F9, 0x571D, 0x5742, 0x5767, 0x578C,
    0x57B0, 0x57D5, 0x57F9, 0x581E, 0x5842, 0x5867, 0x588B, 0x58AF, 0x58D4, 0x58F8, 0x591C, 0x5940,
    0x5964, 0x5988, 0x59AC, 0x59D0, 0x59F3, 0x5A17, 0x5A3B, 0x5A5E, 0x5A82, 0x5AA5, 0x5AC9, 0x5AEC,
    0x5B10, 0x5B33, 0x5B56, 0x5B79, 0x5B9D, 0x5BC0, 0x5BE3, 0x5C06, 0x5C29, 0x5C4B, 0x5C6E, 0x5C91,
    0x5CB4, 0x5CD6, 0x5CF9, 0x5D1B, 0x5D3E, 0x5D60, 0x5D83, 0x5DA5, 0x5DC7, 0x5DE9, 0x5E0B, 0x5E2D,
    0x5E50, 0x5E71, 0x5E93, 0x5EB5, 0x5ED7, 0x5EF9, 0x5F1A, 0x5F3C, 0x5F5E, 0x5F7F, 0x5FA0, 0x5FC2,
    0x5FE3, 0x6004, 0x6026, 0x6047, 0x6068, 0x6089, 0x60AA, 0x60CB, 0x60EC, 0x610D, 0x612D, 0x614E,
    0x616F, 0x618F, 0x61B0, 0x61D0, 0x61F1, 0x6211, 0x6231, 0x6251, 0x6271, 0x6292, 0x62B2, 0x62D2,
    0x62F2, 0x6311, 0x6331, 0x6351, 0x6371, 0x6390, 0x63B0, 0x63CF, 0x63EF, 0x640E, 0x642D, 0x644D,
    0x646C, 0x648B, 0x64AA, 0x64C9, 0x64E8, 0x6507, 0x6526, 0x6545, 0x6563, 0x6582, 0x65A0, 0x65BF,
    0x65DD, 0x65FC, 0x661A, 0x6639, 0x6657, 0x6675, 0x6693, 0x66B1, 0x66CF, 0x66ED, 0x670B, 0x6729,
    0x6746, 0x6764, 0x6782, 0x679F, 0x67BD, 0x67DA, 0x67F7, 0x6815, 0x6832, 0x684F, 0x686C, 0x6889,
    0x68A6, 0x68C3, 0x68E0, 0x68FD, 0x6919, 0x6936, 0x6953, 0x696F, 0x698C, 0x69A8, 0x69C4, 0x69E1,
    0x69FD, 0x6A19, 0x6A35, 0x6A51, 0x6A6D, 0x6A89, 0x6AA5, 0x6AC1, 0x6ADC, 0x6AF8, 0x6B13, 0x6B2F,
    0x6B4A, 0x6B66, 0x6B81, 0x6B9C, 0x6BB8, 0x6BD3, 0x6BEE, 0x6C09, 0x6C24, 0x6C3F, 0x6C59, 0x6C74,
    0x6C8F, 0x6CA9, 0x6CC4, 0x6CDE, 0x6CF9, 0x6D13, 0x6D2D, 0x6D48, 0x6D62, 0x6D7C, 0x6D96, 0x6DB0,
    0x6DCA, 0x6DE3, 0x6DFD, 0x6E17, 0x6E30, 0x6E4A, 0x6E63, 0x6E7D, 0x6E96, 0x6EAF, 0x6EC9, 0x6EE2,
    0x6EFB, 0x6F14, 0x6F2D, 0x6F46, 0x6F5F, 0x6F77, 0x6F90, 0x6FA9, 0x6FC1, 0x6FDA, 0x6FF2, 0x700A,
    0x7023, 0x703B, 0x7053, 0x706B, 0x7083, 0x709B, 0x70B3, 0x70CB, 0x70E2, 0x70FA, 0x7112, 0x7129,
    0x7141, 0x7158, 0x716F, 0x7186, 0x719E, 0x71B5, 0x71CC, 0x71E3, 0x71FA, 0x7211, 0x7227, 0x723E,
    0x7255, 0x726B, 0x7282, 0x7298, 0x72AF, 0x72C5, 0x72DB, 0x72F1, 0x7307, 0x731D, 0x7333, 0x7349,
    0x735F, 0x7375, 0x738A, 0x73A0, 0x73B5, 0x73CB, 0x73E0, 0x73F6, 0x740B, 0x7420, 0x7435, 0x744A,
    0x745F, 0x7474, 0x7489, 0x749E, 0x74B2, 0x74C7, 0x74DB, 0x74F0, 0x7504, 0x7519, 0x752D, 0x7541,
    0x7555, 0x7569, 0x757D, 0x7591, 0x75A5, 0x75B9, 0x75CC, 0x75E0, 0x75F4, 0x7607, 0x761B, 0x762E,
    0x7641, 0x7654, 0x7668, 0x767B, 0x768E, 0x76A0, 0x76B3, 0x76C6, 0x76D9, 0x76EB, 0x76FE, 0x7710,
    0x7723, 0x7735, 0x7747, 0x775A, 0x776C, 0x777E, 0x7790, 0x77A2, 0x77B4, 0x77C5, 0x77D7, 0x77E9,
    0x77FA, 0x780C, 0x781D, 0x782E, 0x7840, 0x7851, 0x7862, 0x7873, 0x7884, 0x7895, 0x78A6, 0x78B6,
    0x78C7, 0x78D8, 0x78E8, 0x78F9, 0x7909, 0x7919, 0x792A, 0x793A, 0x794A, 0x795A, 0x796A, 0x797A,
    0x798A, 0x7999, 0x79A9, 0x79B9, 0x79C8, 0x79D8, 0x79E7, 0x79F6, 0x7A05, 0x7A15, 0x7A24, 0x7A33,
    0x7A42, 0x7A50, 0x7A5F, 0x7A6E, 0x7A7D, 0x7A8B, 0x7A9A, 0x7AA8, 0x7AB6, 0x7AC5, 0x7AD3, 0x7AE1,
    0x7AEF, 0x7AFD, 0x7B0B, 0x7B19, 0x7B26, 0x7B34, 0x7B42, 0x7B4F, 0x7B5D, 0x7B6A, 0x7B77, 0x7B84,
    0x7B92, 0x7B9F, 0x7BAC, 0x7BB9, 0x7BC5, 0x7BD2, 0x7BDF, 0x7BEB, 0x7BF8, 0x7C05, 0x7C11, 0x7C1D,
    0x7C29, 0x7C36, 0x7C42, 0x7C4E, 0x7C5A, 0x7C66, 0x7C71, 0x7C7D, 0x7C89, 0x7C94, 0x7CA0, 0x7CAB,
    0x7CB7, 0x7CC2, 0x7CCD, 0x7CD8, 0x7CE3, 0x7CEE, 0x7CF9, 0x7D04, 0x7D0F, 0x7D19, 0x7D24, 0x7D2F,
    0x7D39, 0x7D43, 0x7D4E, 0x7D58, 0x7D62, 0x7D6C, 0x7D76, 0x7D80, 0x7D8A, 0x7D94, 0x7D9D, 0x7DA7,
    0x7DB0, 0x7DBA, 0x7DC3, 0x7DCD, 0x7DD6, 0x7DDF, 0x7DE8, 0x7DF1, 0x7DFA, 0x7E03, 0x7E0C, 0x7E14,
    0x7E1D, 0x7E26, 0x7E2E, 0x7E37, 0x7E3F, 0x7E47, 0x7E4F, 0x7E57, 0x7E5F, 0x7E67, 0x7E6F, 0x7E77,
    0x7E7F, 0x7E86, 0x7E8E, 0x7E95, 0x7E9D, 0x7EA4, 0x7EAB, 0x7EB3, 0x7EBA, 0x7EC1, 0x7EC8, 0x7ECF,
    0x7ED5, 0x7EDC, 0x7EE3, 0x7EE9, 0x7EF0, 0x7EF6, 0x7EFD, 0x7F03, 0x7F09, 0x7F0F, 0x7F15, 0x7F1B,
    0x7F21, 0x7F27, 0x7F2D, 0x7F32, 0x7F38, 0x7F3D, 0x7F43, 0x7F48, 0x7F4D, 0x7F53, 0x7F58, 0x7F5D,
    0x7F62, 0x7F67, 0x7F6B, 0x7F70, 0x7F75, 0x7F79, 0x7F7E, 0x7F82, 0x7F87, 0x7F8B, 0x7F8F, 0x7F93,
    0x7F97, 0x7F9B, 0x7F9F, 0x7FA3, 0x7FA7, 0x7FAA, 0x7FAE, 0x7FB1, 0x7FB5, 0x7FB8, 0x7FBC, 0x7FBF,
    0x7FC2, 0x7FC5, 0x7FC8, 0x7FCB, 0x7FCE, 0x7FD0, 0x7FD3, 0x7FD6, 0x7FD8, 0x7FDA, 0x7FDD, 0x7FDF,
    0x7FE1, 0x7FE3, 0x7FE5, 0x7FE7, 0x7FE9, 0x7FEB, 0x7FED, 0x7FEE, 0x7FF0, 0x7FF2, 0x7FF3, 0x7FF4,
    0x7FF6, 0x7FF7, 0x7FF8, 0x7FF9, 0x7FFA, 0x7FFB, 0x7FFC, 0x7FFC, 0x7FFD, 0x7FFE, 0x7FFE, 0x7FFF,
    0x7FFF, 0x7FFF, 0x7FFF, 0x8000, 0x8000, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFE, 0x7FFE, 0x7FFD,
    0x7FFC, 0x7FFC, 0x7FFB, 0x7FFA, 0x7FF9, 0x7FF8, 0x7FF7, 0x7FF6, 0x7FF4, 0x7FF3, 0x7FF2, 0x7FF0,
    0x7FEE, 0x7FED, 0x7FEB, 0x7FE9, 0x7FE7, 0x7FE5, 0x7FE3, 0x7FE1, 0x7FDF, 0x7FDD, 0x7FDA, 0x7FD8,
    0x7FD6, 0x7FD3, 0x7FD0, 0x7FCE, 0x7FCB, 0x7FC8, 0x7FC5, 0x7FC2, 0x7FBF, 0x7FBC, 0x7FB8, 0x7FB5,
    0x7FB1, 0x7FAE, 0x7FAA, 0x7FA7, 0x7FA3, 0x7F9F, 0x7F9B, 0x7F97, 0x7F93, 0x7F8F, 0x7F8B, 0x7F87,
    0x7F82, 0x7F7E, 0x7F79, 0x7F75, 0x7F70, 0x7F6B, 0x7F67, 0x7F62, 0x7F5D, 0x7F58, 0x7F53, 0x7F4D,
    0x7F48, 0x7F43, 0x7F3D, 0x7F38, 0x7F32, 0x7F2D, 0x7F27, 0x7F21, 0x7F1B, 0x7F15, 0x7F0F, 0x7F09,
    0x7F03, 0x7EFD, 0x7EF6, 0x7EF0, 0x7EE9, 0x7EE3, 0x7EDC, 0x7ED5, 0x7ECF, 0x7EC8, 0x7EC1, 0x7EBA,
    0x7EB3, 0x7EAB, 0x7EA4, 0x7E9D, 0x7E95, 0x7E8E, 0x7E86, 0x7E7F, 0x7E77, 0x7E6F, 0x7E67, 0x7E5F,
    0x7E57, 0x7E4F, 0x7E47, 0x7E3F, 0x7E37, 0x7E2E, 0x7E26, 0x7E1D, 0x7E14, 0x7E0C, 0x7E03, 0x7DFA,
    0x7DF1, 0x7DE8, 0x7DDF, 0x7DD6, 0x7DCD, 0x7DC3, 0x7DBA, 0x7DB0, 0x7DA7, 0x7D9D, 0x7D94, 0x7D8A,
    0x7D80, 0x7D76, 0x7D6C, 0x7D62, 0x7D58, 0x7D4E, 0x7D43, 0x7D39, 0x7D2F, 0x7D24, 0x7D19, 0x7D0F,
    0x7D04, 0x7CF9, 0x7CEE, 0x7CE3, 0x7CD8, 0x7CCD, 0x7CC2, 0x7CB7, 0x7CAB, 0x7CA0, 0x7C94, 0x7C89,
    0x7C7D, 0x7C71, 0x7C66, 0x7C5A, 0x7C4E, 0x7C42, 0x7C36, 0x7C29, 0x7C1D, 0x7C11, 0x7C05, 0x7BF8,
    0x7BEB, 0x7BDF, 0x7BD2, 0x7BC5, 0x7BB9, 0x7BAC, 0x7B9F, 0x7B92, 0x7B84, 0x7B77, 0x7B6A, 0x7B5D,
    0x7B4F, 0x7B42, 0x7B34, 0x7B26, 0x7B19, 0x7B0B, 0x7AFD, 0x7AEF, 0x7AE1, 0x7AD3, 0x7AC5, 0x7AB6,
    0x7AA8, 0x7A9A, 0x7A8B, 0x7A7D, 0x7A6E, 0x7A5F, 0x7A50, 0x7A42, 0x7A33, 0x7A24, 0x7A15, 0x7A05,
    0x79F6, 0x79E7, 0x79D8, 0x79C8, 0x79B9, 0x79A9, 0x7999, 0x798A, 0x797A, 0x796A, 0x795A, 0x794A,
    0x793A, 0x792A, 0x7919, 0x7909, 0x78F9, 0x78E8, 0x78D8, 0x78C7, 0x78B6, 0x78A6, 0x7895, 0x7884,
    0x7873, 0x7862, 0x7851, 0x7840, 0x782E, 0x781D, 0x780C, 0x77FA, 0x77E9, 0x77D7, 0x77C5, 0x77B4,
    0x77A2, 0x7790, 0x777E, 0x776C, 0x775A, 0x7747, 0x7735, 0x7723, 0x7710, 0x76FE, 0x76EB, 0x76D9,
    0x76C6, 0x76B3, 0x76A0, 0x768E, 0x767B, 0x7668, 0x7654, 0x7641, 0x762E, 0x761B, 0x7607, 0x75F4,
    0x75E0, 0x75CC, 0x75B9, 0x75A5, 0x7591, 0x757D, 0x7569, 0x7555, 0x7541, 0x752D, 0x7519, 0x7504,
    0x74F0, 0x74DB, 0x74C7, 0x74B2, 0x749E, 0x7489, 0x7474, 0x745F, 0x744A, 0x7435, 0x7420, 0x740B,
    0x73F6, 0x73E0, 0x73CB, 0x73B5, 0x73A0, 0x738A, 0x7375, 0x735F, 0x7349, 0x7333, 0x731D, 0x7307,
    0x72F1, 0x72DB, 0x72C5, 0x72AF, 0x7298, 0x7282, 0x726B, 0x7255, 0x723E, 0x7227, 0x7211, 0x71FA,
    0x71E3, 0x71CC, 0x71B5, 0x719E, 0x7186, 0x716F, 0x7158, 0x7141, 0x7129, 0x7112, 0x70FA, 0x70E2,
    0x70CB, 0x70B3, 0x709B, 0x7083, 0x706B, 0x7053, 0x703B, 0x7023, 0x700A, 0x6FF2, 0x6FDA, 0x6FC1,
    0x6FA9, 0x6F90, 0x6F77, 0x6F5F, 0x6F46, 0x6F2D, 0x6F14, 0x6EFB, 0x6EE2, 0x6EC9, 0x6EAF, 0x6E96,
    0x6E7D, 0x6E63, 0x6E4A, 0x6E30, 0x6E17, 0x6DFD, 0x6DE3, 0x6DCA, 0x6DB0, 0x6D96, 0x6D7C, 0x6D62,
    0x6D48, 0x6D2D, 0x6D13, 0x6CF9, 0x6CDE, 0x6CC4, 0x6CA9, 0x6C8F, 0x6C74, 0x6C59, 0x6C3F, 0x6C24,
    0x6C09, 0x6BEE, 0x6BD3, 0x6BB8, 0x6B9C, 0x6B81, 0x6B66, 0x6B4A, 0x6B2F, 0x6B13, 0x6AF8, 0x6ADC,
    0x6AC1, 0x6AA5, 0x6A89, 0x6A6D, 0x6A51, 0x6A35, 0x6A19, 0x69FD, 0x69E1, 0x69C4, 0x69A8, 0x698C,
    0x696F, 0x6953, 0x6936, 0x6919, 0x68FD, 0x68E0, 0x68C3, 0x68A6, 0x6889, 0x686C, 0x684F, 0x6832,
    0x6815, 0x67F7, 0x67DA, 0x67BD, 0x679F, 0x6782, 0x6764, 0x6746, 0x6729, 0x670B, 0x66ED, 0x66CF,
    0x66B1, 0x6693, 0x6675, 0x6657, 0x6639, 0x661A, 0x65FC, 0x65DD, 0x65BF, 0x65A0, 0x6582, 0x6563,
    0x6545, 0x6526, 0x6507, 0x64E8, 0x64C9, 0x64AA, 0x648B, 0x646C, 0x644D, 0x642D, 0x640E, 0x63EF,
    0x63CF, 0x63B0, 0x6390, 0x6371, 0x6351, 0x6331, 0x6311, 0x62F2, 0x62D2, 0x62B2, 0x6292, 0x6271,
    0x6251, 0x6231, 0x6211, 0x61F1, 0x61D0, 0x61B0, 0x618F, 0x616F, 0x614E, 0x612D, 0x610D, 0x60EC,
    0x60CB, 0x60AA, 0x6089, 0x6068, 0x6047, 0x6026, 0x6004, 0x5FE3, 0x5FC2, 0x5FA0, 0x5F7F, 0x5F5E,
    0x5F3C, 0x5F1A, 0x5EF9, 0x5ED7, 0x5EB5, 0x5E93, 0x5E71, 0x5E50, 0x5E2D, 0x5E0B, 0x5DE9, 0x5DC7,
    0x5DA5, 0x5D83, 0x5D60, 0x5D3E, 0x5D1B, 0x5CF9, 0x5CD6, 0x5CB4, 0x5C91, 0x5C6E, 0x5C4B, 0x5C29,
    0x5C06, 0x5BE3, 0x5BC0, 0x5B9D, 0x5B79, 0x5B56, 0x5B33, 0x5B10, 0x5AEC, 0x5AC9, 0x5AA5, 0x5A82,
    0x5A5E, 0x5A3B, 0x5A17, 0x59F3, 0x59D0, 0x59AC, 0x5988, 0x5964, 0x5940, 0x591C, 0x58F8, 0x58D4,
    0x58AF, 0x588B, 0x5867, 0x5842, 0x581E, 0x57F9, 0x57D5, 0x57B0, 0x578C, 0x5767, 0x5742, 0x571D,
    0x56F9, 0x56D4, 0x56AF, 0x568A, 0x5665, 0x5640, 0x561A, 0x55F5, 0x55D0, 0x55AB, 0x5585, 0x5560,
    0x553A, 0x5515, 0x54EF, 0x54CA, 0x54A4, 0x547E, 0x5458, 0x5433, 0x540D, 0x53E7, 0x53C1, 0x539B,
    0x5375, 0x534E, 0x5328, 0x5302, 0x52DC, 0x52B5, 0x528F, 0x5269, 0x5242, 0x521C, 0x51F5, 0x51CE,
    0x51A8, 0x5181, 0x515A, 0x5133, 0x510C, 0x50E5, 0x50BF, 0x5097, 0x5070, 0x5049, 0x5022, 0x4FFB,
    0x4FD4, 0x4FAC, 0x4F85, 0x4F5E, 0x4F36, 0x4F0F, 0x4EE7, 0x4EBF, 0x4E98, 0x4E70, 0x4E48, 0x4E21,
    0x4DF9, 0x4DD1, 0x4DA9, 0x4D81, 0x4D59, 0x4D31, 0x4D09, 0x4CE1, 0x4CB8, 0x4C90, 0x4C68, 0x4C3F,
    0x4C17, 0x4BEF, 0x4BC6, 0x4B9E, 0x4B75, 0x4B4C, 0x4B24, 0x4AFB, 0x4AD2, 0x4AA9, 0x4A81, 0x4A58,
    0x4A2F, 0x4A06, 0x49DD, 0x49B4, 0x498A, 0x4961, 0x4938, 0x490F, 0x48E6, 0x48BC, 0x4893, 0x4869,
    0x4840, 0x4816, 0x47ED, 0x47C3, 0x479A, 0x4770, 0x4746, 0x471C, 0x46F3, 0x46C9, 0x469F, 0x4675,
    0x464B, 0x4621, 0x45F7, 0x45CD, 0x45A3, 0x4578, 0x454E, 0x4524, 0x44FA, 0x44CF, 0x44A5, 0x447A,
    0x4450, 0x4425, 0x43FB, 0x43D0, 0x43A5, 0x437B, 0x4350, 0x4325, 0x42FA, 0x42D0, 0x42A5, 0x427A,
    0x424F, 0x4224, 0x41F9, 0x41CE, 0x41A2, 0x4177, 0x414C, 0x4121, 0x40F6, 0x40CA, 0x409F, 0x4073,
    0x4048, 0x401D, 0x3FF1, 0x3FC5, 0x3F9A, 0x3F6E, 0x3F43, 0x3F17, 0x3EEB, 0x3EBF, 0x3E93, 0x3E68,
    0x3E3C, 0x3E10, 0x3DE4, 0x3DB8, 0x3D8C, 0x3D60, 0x3D33, 0x3D07, 0x3CDB, 0x3CAF, 0x3C83, 0x3C56,
    0x3C2A, 0x3BFD, 0x3BD1, 0x3BA5, 0x3B78, 0x3B4C, 0x3B1F, 0x3AF2, 0x3AC6, 0x3A99, 0x3A6C, 0x3A40,
    0x3A13, 0x39E6, 0x39B9, 0x398C, 0x395F, 0x3932, 0x3906, 0x38D8, 0x38AB, 0x387E, 0x3851, 0x3824,
    0x37F7, 0x37CA, 0x379C, 0x376F, 0x3742, 0x3714, 0x36E7, 0x36BA, 0x368C, 0x365F, 0x3631, 0x3604,
    0x35D6, 0x35A8, 0x357B, 0x354D, 0x351F, 0x34F2, 0x34C4, 0x3496, 0x3468, 0x343A, 0x340C, 0x33DE,
    0x33B0, 0x3382, 0x3354, 0x3326, 0x32F8, 0x32CA, 0x329C, 0x326E, 0x3240, 0x3211, 0x31E3, 0x31B5,
    0x3186, 0x3158, 0x312A, 0x30FB, 0x30CD, 0x309E, 0x3070, 0x3041, 0x3013, 0x2FE4, 0x2FB5, 0x2F87,
    0x2F58, 0x2F29, 0x2EFB, 0x2ECC, 0x2E9D, 0x2E6E, 0x2E3F, 0x2E11, 0x2DE2, 0x2DB3, 0x2D84, 0x2D55,
    0x2D26, 0x2CF7, 0x2CC8, 0x2C98, 0x2C69, 0x2C3A, 0x2C0B, 0x2BDC, 0x2BAD, 0x2B7D, 0x2B4E, 0x2B1F,
    0x2AEF, 0x2AC0, 0x2A91, 0x2A61, 0x2A32, 0x2A02, 0x29D3, 0x29A3, 0x2974, 0x2944, 0x2915, 0x28E5,
    0x28B5, 0x2886, 0x2856, 0x2826, 0x27F6, 0x27C7, 0x2797, 0x2767, 0x2737, 0x2707, 0x26D8, 0x26A8,
    0x2678, 0x2648, 0x2618, 0x25E8, 0x25B8, 0x2588, 0x2558, 0x2528, 0x24F7, 0x24C7, 0x2497, 0x2467,
    0x2437, 0x2407, 0x23D6, 0x23A6, 0x2376, 0x2345, 0x2315, 0x22E5, 0x22B4, 0x2284, 0x2254, 0x2223,
    0x21F3, 0x21C2, 0x2192, 0x2161, 0x2131, 0x2100, 0x20D0, 0x209F, 0x206E, 0x203E, 0x200D, 0x1FDC,
    0x1FAC, 0x1F7B, 0x1F4A, 0x1F19, 0x1EE9, 0x1EB8, 0x1E87, 0x1E56, 0x1E25, 0x1DF5, 0x1DC4, 0x1D93,
    0x1D62, 0x1D31, 0x1D00, 0x1CCF, 0x1C9E, 0x1C6D, 0x1C3C, 0x1C0B, 0x1BDA, 0x1BA9, 0x1B78, 0x1B47,
    0x1B16, 0x1AE4, 0x1AB3, 0x1A82, 0x1A51, 0x1A20, 0x19EF, 0x19BD, 0x198C, 0x195B, 0x192A, 0x18F8,
    0x18C7, 0x1896, 0x1864, 0x1833, 0x1802, 0x17D0, 0x179F, 0x176D, 0x173C, 0x170A, 0x16D9, 0x16A8,
    0x1676, 0x1645, 0x1613, 0x15E2, 0x15B0, 0x157F, 0x154D, 0x151B, 0x14EA, 0x14B8, 0x1487, 0x1455,
    0x1423, 0x13F2, 0x13C0, 0x138E, 0x135D, 0x132B, 0x12F9, 0x12C8, 0x1296, 0x1264, 0x1232, 0x1201,
    0x11CF, 0x119D, 0x116B, 0x1139, 0x1108, 0x10D6, 0x10A4, 0x1072, 0x1040, 0x100E, 0x0FDD, 0x0FAB,
    0x0F79, 0x0F47, 0x0F15, 0x0EE3, 0x0EB1, 0x0E7F, 0x0E4D, 0x0E1B, 0x0DE9, 0x0DB7, 0x0D85, 0x0D53,
    0x0D21, 0x0CEF, 0x0CBD, 0x0C8B, 0x0C59, 0x0C27, 0x0BF5, 0x0BC3, 0x0B91, 0x0B5F, 0x0B2D, 0x0AFB,
    0x0AC9, 0x0A97, 0x0A65, 0x0A33, 0x0A00, 0x09CE, 0x099C, 0x096A, 0x0938, 0x0906, 0x08D4, 0x08A2,
    0x086F, 0x083D, 0x080B, 0x07D9, 0x07A7, 0x0775, 0x0742, 0x0710, 0x06DE, 0x06AC, 0x067A, 0x0647,
    0x0615, 0x05E3, 0x05B1, 0x057F, 0x054C, 0x051A, 0x04E8, 0x04B6, 0x0483, 0x0451, 0x041F, 0x03ED,
    0x03BA, 0x0388, 0x0356, 0x0324, 0x02F1, 0x02BF, 0x028D, 0x025B, 0x0228, 0x01F6, 0x01C4, 0x0192,
    0x015F, 0x012D, 0x00FB, 0x00C9, 0x0096, 0x0064, 0x0032, 0x0000
};


#define PI32 3.1415927F
#define SINTABLE_RAD_TO_ID(x)      ((s32)((x) * ((f32)ARRAY_COUNT(gSYSinTable) / PI32)))
#define syGetSinCosUShort(sin, cos, angle, id) 	\
{                                               \
    id = SINTABLE_RAD_TO_ID(angle) & 0xFFF;     \
                                                \
    sin = gSYSinTable[id & 0x7FF];     			\
                                                \
    if (id & 0x800)                             \
    {                                           \
        sin = -sin;                             \
    }                                          	\
    id += 0x400;                               	\
                                    			\
    cos = gSYSinTable[id & 0x7FF];     			\
                                                \
    if (id & 0x800)                             \
    {                                           \
        cos = -cos;                             \
    }                                           \
}
#define COMBINE_INTEGRAL(a, b)   (((a)&0xffff0000) | (((b) >> 16)))
#define COMBINE_FRACTIONAL(a, b) (((a) << 16) | ((b)&0xffff))

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