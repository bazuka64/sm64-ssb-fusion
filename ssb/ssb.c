#include <macros.h>
#include <PR/gbi.h>
#include <PR/mbi.h>
#include <PR/gu.h>
extern Gfx* gDisplayListHead;

typedef struct Controller {
    /*0x00*/ s16 rawStickX;               // Analog stick [-128, 128] positive is right. Used for menus.
    /*0x02*/ s16 rawStickY;               // Analog stick [-128, 128] positive is up. Used for menus.
    /*0x04*/ f32 stickX;                  // Analog stick [-64, 64] positive is right. Used for gameplay.
    /*0x08*/ f32 stickY;                  // Analog stick [-64, 64] positive is up. Used for gameplay.
    /*0x0C*/ f32 stickMag;                // Analog stick distance from center [0, 64]. Used for gameplay.
    /*0x10*/ u16 buttonDown;              // Buttons held down on the current frame.
    /*0x12*/ u16 buttonPressed;           // Buttons pressed on the current frame but not held on the previous frame.
    /*0x14*/ u16 buttonReleased;          // Burrons released on the current frame and held on the previous frame.
    /*0x18*/ void* statusData;    // Pointer to the controller status data in gControllerStatuses.
    /*0x1C*/ void* controllerData; // Pointer to the raw input data in gControllerPads.
    /*0x20*/ s32 port;                    // The port index this controller is plugged into [0, 3].
} Controller; /*0x24*/
extern struct Controller gControllers[4];

#define CONT_A      0x8000
#define CONT_B      0x4000
#define CONT_G	    0x2000
#define CONT_START  0x1000
#define CONT_UP     0x0800
#define CONT_DOWN   0x0400
#define CONT_LEFT   0x0200
#define CONT_RIGHT  0x0100
#define CONT_L      0x0020
#define CONT_R      0x0010
#define CONT_E      0x0008
#define CONT_D      0x0004
#define CONT_C      0x0002
#define CONT_F      0x0001

/* Nintendo's official button names */

#define A_BUTTON	CONT_A
#define B_BUTTON	CONT_B
#define L_TRIG		CONT_L
#define R_TRIG		CONT_R
#define Z_TRIG		CONT_G
#define START_BUTTON	CONT_START
#define U_JPAD		CONT_UP
#define L_JPAD		CONT_LEFT
#define R_JPAD		CONT_RIGHT
#define D_JPAD		CONT_DOWN
#define U_CBUTTONS	CONT_E
#define L_CBUTTONS	CONT_C
#define R_CBUTTONS	CONT_F
#define D_CBUTTONS	CONT_D
#define GCN_X_BUTTON 0x0040
#define GCN_Y_BUTTON 0x0080

#include "lbdef.h"
#include "ftdef.h"
#include "objdef.h"
#include "mpdef.h"
#include "gmdef.h"

#include "struct.h"
#include "jittai.inc.h"
#include "debug.h"

#include "ssb_memory.inc.h"
#include "dobj.inc.h"
#include "anim.inc.h"

SYController cont;

void ftMainSetStatus(GObj* fighter_gobj, s32 status_id, f32 frame_begin, f32 anim_speed, u32 flags);

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

void ftMainSetStatus(GObj* fighter_gobj, s32 status_id, f32 frame_begin, f32 anim_speed, u32 flags);

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

#define ftCommonGroundCheckInterrupt(fighter_gobj) \
(	\
	(ftCommonAppealCheckInterruptCommon(fighter_gobj) != FALSE) \
)

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
