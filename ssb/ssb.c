// mario
#include <macros.h>
#include <PR/gbi.h>
#include <PR/mbi.h>
#include <PR/gu.h>
extern Gfx* gDisplayListHead;

// ssb def
#include "lbdef.h"
#include "ftdef.h"
#include "objdef.h"
#include "mpdef.h"

// sturct jittai
#include "struct.h"
#include "jittai.inc.h"

// debug
int debug_value = 0;
int first = 0;
int count = 0;
int count2 = 0;
void append_puppyprint_log(const char* str, ...);
// print_text_fmt_int(10, 10, "%d", debug_value);

// func
void syDebugPrintf(const char* fmt, ...) {}
void scManagerRunPrintGObjStatus() {}

// inc.h
#include "ssb_memory.inc.h"
#include "dobj.inc.h"

void ssb_init() {

    FTAttributes* attr;
    DObj* topn_joint;
    GObj* fighter_gobj = &gobj;
    FTStruct* fp;

    syMallocInit(&gSYTaskmanGeneralHeap, 0x10000, 0x80400000, 0x400000);
    syMallocInit(&gSYTaskmanGraphicsHeap, 0x10002, syTaskmanMalloc(0xD000, 0x8), 0xD000);

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
}

void ssb_draw(Vec3fArray pos) {

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

    // print_text_fmt_int(10, 10, "%d", count);
}
