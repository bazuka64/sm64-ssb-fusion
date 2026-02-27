#include <macros.h>
#include <PR/gbi.h>
#include <PR/mbi.h>
#include <PR/gu.h>
#include "ssb_mario.h"
extern Gfx* gDisplayListHead;
extern struct Controller gControllers[4];

#include "lbdef.h"
#include "ftdef.h"
#include "objdef.h"
#include "mpdef.h"
#include "gmdef.h"

#include "define.h"
#include "struct.h"
#include "jittai.inc.h"
#include "proto.h"
#include "debug.h"

#include "ssb_memory.inc.h"
#include "dobj.inc.h"
#include "anim.inc.h"
#include "status.inc.h"
#include "ssb_sub.inc.h"

void ssb_init() {

    FTAttributes* attr;
    DObj* topn_joint;
    GObj* fighter_gobj = &gobj;
    FTStruct* fp;
    FTMotionDesc* motion_desc;
    FTMotionDescArray* script_array;

    syMallocInit(&gSYTaskmanGeneralHeap, 0x10000, 0x80400000, 0x400000);
    syMallocInit(&gSYTaskmanGraphicsHeap, 0x10002, syTaskmanMalloc(0xD000, 0x8), 0xD000);

    scVSBattleSetupFiles();
    ftManagerSetupFilesAllKind(nFTKindNess);

    // make figher
    gobj.user_data.p = &fighter;
    fp = &fighter;
    fp->data = &dFTNessData;
    fp->input.controller = &cont;
    fp->input.button_mask_a = A_BUTTON;
    fp->input.button_mask_b = B_BUTTON;
    fp->input.button_mask_z = Z_TRIG;
    fp->input.button_mask_l = L_TRIG;
    attr = lbRelocGetFileData(FTAttributes*, *dFTNessData.p_file_main, dFTNessData.o_attributes);
    for (int i = 0; i < ARRAY_COUNT(fp->joints); i++)
    {
        fp->joints[i] = NULL;
    }
    topn_joint = gcAddDObjForGObj(fighter_gobj, NULL);
    fp->joints[nFTPartsJointTopN] = topn_joint;
    lbCommonInitDObj3Transforms(topn_joint, 0x4B, nGCMatrixKindNull, nGCMatrixKindNull);
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

    fp->figatree_heap = ftManagerAllocFigatreeHeapKind(nFTKindNess);
    fp->figatree = fp->figatree_heap;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusWait, 0.0f, 1.0f, 0);
}

void ssb_draw(Vec3fArray pos) {

    ftMainProcUpdateInterrupt(&gobj);
    ftMainProcPhysicsMap(&gobj);

    syMallocReset(&gSYTaskmanGraphicsHeap);

    // テクスチャをつけるのに必要
    gDPPipeSync(gDisplayListHead++);
    gDPSetCycleType(gDisplayListHead++, G_CYC_2CYCLE);
    gSPSetGeometryMode(gDisplayListHead++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    gDPSetRenderMode(gDisplayListHead++, G_RM_FOG_PRIM_A, G_RM_AA_ZB_OPA_SURF2);

    Mtx* mtx = alloc_display_list(sizeof(*mtx)); // s32
    Mat4 mat4; // f32
    mtxf_translate(mat4, pos);
    mtxf_to_mtx_fast(mtx, mat4);

    gSPMatrix(
        gDisplayListHead++,
        VIRTUAL_TO_PHYSICAL(mtx),
        G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
    );

    ftDisplayMainDrawDefault(DObjGetStruct(&gobj));
}
