#define lbRelocGetFileData(type, file, offset) \
((type) ((uintptr_t)(file) + (intptr_t)(offset)))

typedef f32 Mat4[4][4];
typedef f32 Vec3fArray[3]; // X, Y, Z, where Y is up

#define DOBJ_PARENT_NULL            ((DObj*)1)
#define DOBJ_FLAG_NONE              (0)
#define DOBJ_FLAG_HIDDEN            (1 << 1)
#define F32_MAX 3.40282346639e+38F
#define F32_MIN (-F32_MAX)
#define AOBJ_ANIM_NULL              F32_MIN      
#define AOBJ_ANIM_CHANGED           (F32_MIN / 2.0F)    // ???
#define AOBJ_ANIM_END               (F32_MIN / 3.0F)    // Used to 
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

#define ftGetStruct(fighter_gobj) 	((FTStruct*)(fighter_gobj)->user_data.p)
#define AObjAnimAdvance(script) \
((script)++)

#define GOBJ_FLAG_NOANIM            (1 << 1)            // Skip applying aninmation values?
#define SQUARE(x) ((x) * (x))

#define ftCommonGroundCheckInterrupt(fighter_gobj) \
(	\
	(ftCommonAppealCheckInterruptCommon(fighter_gobj) != FALSE) \
)