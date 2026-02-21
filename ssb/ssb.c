#include <macros.h>
#include <PR/gbi.h>
#include <PR/mbi.h>
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

#define DOBJ_PARENT_NULL            ((DObj*)1)
#define DOBJ_FLAG_NONE              (0)
#define DOBJ_FLAG_HIDDEN            (1 << 1)
#define F32_MAX 3.40282346639e+38F
#define F32_MIN (-F32_MAX)
#define AOBJ_ANIM_NULL              F32_MIN      
#define DObjGetStruct(gobj) ((DObj*)(gobj)->obj)
#define DOBJ_ARRAY_MAX              18
#define NBITS(t) ((int) (sizeof(t) * 8) )

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
    // for (int i = 0; i < 1; i++){
    //     syMallocInit(&gSYTaskmanGraphicsHeap, 0x10002, syTaskmanMalloc(0xD000, 0x8), 0xD000);
    // }
    for (int i = 0; i < 2; i++)
    {
        syMallocInit(&gSYTaskmanGraphicsHeap, 0x10002, syTaskmanMalloc(0xD000, 0x8), 0xD000);
        sSYTaskmanDefaultGraphicsHeap[i].id = gSYTaskmanGraphicsHeap.id;
        sSYTaskmanDefaultGraphicsHeap[i].start = gSYTaskmanGraphicsHeap.start;
        sSYTaskmanDefaultGraphicsHeap[i].end = gSYTaskmanGraphicsHeap.end;
        sSYTaskmanDefaultGraphicsHeap[i].ptr = gSYTaskmanGraphicsHeap.ptr;
    }

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
    MObj* mobj;
    Gfx* new_dl;
    Gfx* branch_dl;
    if (dobj->mobj == NULL)
    {
        return;
    }
    gSPSegment((*dl_head)++, 0xE, gSYTaskmanGraphicsHeap.ptr);
    for (mobj_count = 0, mobj = dobj->mobj; mobj != NULL; mobj_count++)
    {
        mobj = mobj->next;
    }
    // append_puppyprint_log("mobj_count: %d", mobj_count);
    mobj = dobj->mobj;
    branch_dl = (Gfx*)(((Gfx*)gSYTaskmanGraphicsHeap.ptr) + mobj_count);
    new_dl = gSYTaskmanGraphicsHeap.ptr;

    for (int i = 0; i < mobj_count; i++, mobj = mobj->next) {
        // gSPBranchList(&new_dl[i], branch_dl); // テクスチャつけるときはこれ
        gSPEndDisplayList(&new_dl[i]);
    }
    gSYTaskmanGraphicsHeap.ptr = (void*)branch_dl;

}

void ftDisplayMainDrawDefault(DObj* dobj) {
    DObj* sibling_dobj;

    if (dobj->dl != 0) {

         {
            gcDrawMObjForDObj(dobj, &gDisplayListHead);
            
            // for(int i=0;i<20;i++){
            //     if(dobj->dl[i].words.w0 == 0xDE000000){
            //         append_puppyprint_log("BranchList to %x", dobj->dl[i].words.w1);
            //     }
            // }

            gSPDisplayList(gDisplayListHead++, dobj->dl);
        }
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

void ssb_draw() {
    if (gSYTaskmanTaskID == 1)gSYTaskmanTaskID = 0;
    else gSYTaskmanTaskID = 1;
    syTaskmanResetGraphicsHeap();
    ftDisplayMainProcDisplay(&gobj);
    // print_text_fmt_int(10, 10, "%d", debug_value);
    first = 1;
}
