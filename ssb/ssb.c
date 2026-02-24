#include <macros.h>
#include <PR/gbi.h>
#include <PR/mbi.h>
#include <PR/gu.h>
extern Gfx* gDisplayListHead;

#include "lbdef.h"
#include "ftdef.h"
#include "objdef.h"
#include "mpdef.h"

#include "struct.h"
#include "jittai.inc.h"

// debug
int debug_value = 0;
int first = 0;
int count = 0;
void append_puppyprint_log(const char* str, ...);
// print_text_fmt_int(10, 10, "%d", debug_value);

// func
void syDebugPrintf(const char* fmt, ...) {}
void scManagerRunPrintGObjStatus() {}
#include "ssb_memory.inc.h"

#define lbRelocGetFileData(type, file, offset) \
((type) ((uintptr_t)(file) + (intptr_t)(offset)))

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

static Lights1 sSSBDefaultLights = gdSPDefLights1(
    0x40, 0x40, 0x40,
    0xFF, 0xFF, 0xFF, 0x28, 0x28, 0x28
);

static Mtx sSSBViewMtx;
static Mtx sSSBModelMtx;

static const f32 sSSBCamEyeX = 0.0f;
static const f32 sSSBCamEyeY = 120.0f;
static const f32 sSSBCamEyeZ = 450.0f;
static const f32 sSSBCamAtX = 0.0f;
static const f32 sSSBCamAtY = 80.0f;
static const f32 sSSBCamAtZ = 0.0f;
static const f32 sSSBPosX = 0.0f;
static const f32 sSSBPosY = 0.0f;
static const f32 sSSBPosZ = 0.0f;

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

void ssb_init() {
    FTAttributes* attr;
    DObj* topn_joint;
    GObj* fighter_gobj = &gobj;
    FTStruct* fp;

    syMallocInit(&gSYTaskmanGeneralHeap, 0x10000, 0x80400000, 0x400000);

    syMallocInit(&gSYTaskmanGraphicsHeap, 0x10002, syTaskmanMalloc(0xD000, 0x8), 0xD000);
    // for (int i = 0; i < 2; i++)
    // {
    //     syMallocInit(&gSYTaskmanGraphicsHeap, 0x10002, syTaskmanMalloc(0xD000, 0x8), 0xD000);
    //     sSYTaskmanDefaultGraphicsHeap[i].id = gSYTaskmanGraphicsHeap.id;
    //     sSYTaskmanDefaultGraphicsHeap[i].start = gSYTaskmanGraphicsHeap.start;
    //     sSYTaskmanDefaultGraphicsHeap[i].end = gSYTaskmanGraphicsHeap.end;
    //     sSYTaskmanDefaultGraphicsHeap[i].ptr = gSYTaskmanGraphicsHeap.ptr;
    // }

    scVSBattleSetupFiles();
    ftManagerSetupFilesAllKind(nFTKindNess);

    fp = &fighter;
    attr = lbRelocGetFileData(FTAttributes*, *dFTNessData.p_file_main, dFTNessData.o_attributes);
    for (int i = 0; i < ARRAY_COUNT(fp->joints); i++)
    {
        fp->joints[i] = NULL;
    }
    topn_joint = gcAddDObjForGObj(fighter_gobj, NULL);
    fp->joints[nFTPartsJointTopN] = topn_joint;
    lbCommonSetupFighterPartsDObjs
    (
        DObjGetStruct(fighter_gobj),
        attr->commonparts_container,
        1/*fp->detail_curr*/,// high poly
        &fp->joints[nFTPartsJointCommonStart],
        attr->setup_parts,
        0x4B,
        nGCMatrixKindNull,
        nGCMatrixKindNull,
        fp->costume,
        fp->unk_ft_0x149
    );
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

void ftDisplayMainDrawDefault(DObj* dobj) {
    DObj* sibling_dobj;

    if (dobj->dl != 0) {

        gcDrawMObjForDObj(dobj, &gDisplayListHead);

        // gDPPipeSync(gDisplayListHead++);
        // gDPSetRenderMode(gDisplayListHead++, G_RM_PASS, G_RM_AA_ZB_OPA_SURF2);

        gSPDisplayList(gDisplayListHead++, dobj->dl);
    }

    if (dobj->child != NULL)
    {
        ftDisplayMainDrawDefault(dobj->child);
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

void ftDisplayMainDrawAll(GObj* fighter_gobj) {
    ftDisplayMainDrawDefault(DObjGetStruct(fighter_gobj));
}

void ftDisplayMainProcDisplay(GObj* fighter_gobj) {
    ftDisplayMainDrawAll(fighter_gobj);
}

void syMallocReset(SYMallocRegion* bp)
{
    bp->ptr = bp->start;
}

void syTaskmanResetGraphicsHeap(void) // reset gSYTaskmanGraphicsHeap allocator
{
    gSYTaskmanGraphicsHeap.id = sSYTaskmanDefaultGraphicsHeap[gSYTaskmanTaskID].id;
    gSYTaskmanGraphicsHeap.start = sSYTaskmanDefaultGraphicsHeap[gSYTaskmanTaskID].start;
    gSYTaskmanGraphicsHeap.end = sSYTaskmanDefaultGraphicsHeap[gSYTaskmanTaskID].end;
    gSYTaskmanGraphicsHeap.ptr = sSYTaskmanDefaultGraphicsHeap[gSYTaskmanTaskID].ptr;

    syMallocReset(&gSYTaskmanGraphicsHeap);
}

typedef f32 Mat4[4][4];
typedef f32 Vec3fArray[3]; // X, Y, Z, where Y is up

void ssb_draw(Vec3fArray pos) {

    // if (gSYTaskmanTaskID == 1)gSYTaskmanTaskID = 0;
    // else gSYTaskmanTaskID = 1;
    //syTaskmanResetGraphicsHeap();
    syMallocReset(&gSYTaskmanGraphicsHeap);

    // テクスチャをつけるのに必要
    gDPPipeSync(gDisplayListHead++);
    gDPSetCycleType(gDisplayListHead++, G_CYC_2CYCLE);
    gSPSetGeometryMode(gDisplayListHead++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    gDPSetRenderMode(gDisplayListHead++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);

    Mtx* mtx = alloc_display_list(sizeof(*mtx)); // s32
    Mat4 mat4;
    mtxf_translate(mat4, pos);
    mtxf_to_mtx_fast(mtx, mat4);

    gSPMatrix(
        gDisplayListHead++,
        VIRTUAL_TO_PHYSICAL(mtx),
        G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
    );

    ftDisplayMainProcDisplay(&gobj);
}

