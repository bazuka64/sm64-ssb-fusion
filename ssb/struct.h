#pragma once
#include "ssb_types.h"

struct LBRelocSetup
{
    uintptr_t table_addr;        // ROM address
    u32 table_files_num;         // Total number of files in table?
    void *file_heap;         
    size_t file_heap_size;
    LBFileNode *status_buffer;
    size_t status_buffer_size;
    LBFileNode *force_status_buffer;
    size_t force_status_buffer_size;
};

struct LBInternBuffer
{
    uintptr_t rom_table_lo; // Start of file table
    u32 total_files_num;
    uintptr_t rom_table_hi; // End of file table
    void *heap_start;
    void *heap_ptr;
    void *heap_end;

    // "status buffer"
    s32 status_buffer_num;
    s32 status_buffer_max;
    LBFileNode *status_buffer;

    // "force status buffer"
    s32 force_status_buffer_num;
    s32 force_status_buffer_max;
    LBFileNode *force_status_buffer;
};

struct LBTableEntry
{
    ub32 is_compressed : 1;
    u32 data_offset : 31;
    u16 reloc_intern_offset;    // in words
    u16 compressed_size;        // in words
    u16 reloc_extern_offset;    // in words
    u16 decompressed_size;      // in words
};

struct LBFileNode
{
    u32 id;
    void *addr;
};

typedef struct SYMallocRegion 
{
    u32 id;
    void *start;
    void *end;
    void *ptr;

} SYMallocRegion;

struct FTData
{
    u32 file_main_id; // File size in bytes?
    u32 file_mainmotion_id;
    u32 file_submotion_id;
    u32 file_model_id;
    u32 file_shieldpose_id;
    u32 file_special1_id;
    u32 file_special2_id;
    u32 file_special3_id;
    u32 file_special4_id;
    size_t file_main_size;
    void **p_file_main; // Pointer to character's file?
    void **p_file_mainmotion;
    void **p_file_submotion;
    void **p_file_model;
    void **p_file_shieldpose;
    void **p_file_special1;
    void **p_file_special2;
    void **p_file_special3;
    void **p_file_special4;
    s32 *p_particle;
    uintptr_t particles_script_lo;
    uintptr_t particles_script_hi;
    uintptr_t particles_texture_lo;
    uintptr_t particles_texture_hi;
    intptr_t o_attributes; // Offset to fighter's attributes
    FTMotionDescArray *mainmotion;
    FTMotionDescArray *submotion;
    s32 mainmotion_array_count;
    s32 *submotion_array_count;
    size_t file_anim_size;
};

typedef struct SYHuffmanNode
{
    struct SYHuffmanNode *left;
    struct SYHuffmanNode *right;
    s32 value;

} SYHuffmanNode;

union LBRelocDesc
{
    struct lbRelocInfo
    {
        u16 reloc;
        u16 words_num;

    } info;

    void *p;
};

struct GObjScript
{
    GObj *next_gobj;
    s32 id;
};

union GCUserData
{
    s32 s;
    u32 u;
    void *p;
};

struct GObj
{
    u32 id;
    GObj *link_next;
    GObj *link_prev;
    u8 link_id;
    u8 dl_link_id;
    u8 frame_draw_last;                 // Last frame drawn?
    u8 obj_kind;                        // Determines kind of *obj: 0 = NULL, 1 = DObj, 2 = SObj, 3 = CObj
    u32 link_priority;
    void (*func_run)(GObj*);
    GObjProcess *gobjproc_head;

    union
    {
        s32 unk_0x1C;
        GObj *unk_gobj_0x1C;
        GObjProcess *gobjproc_tail;
    };
    
    GObj *dl_link_next;
    GObj *dl_link_prev;
    u32 dl_link_priority;
    void (*proc_display)(GObj*);
    u64 camera_mask;
    u32 camera_tag;                       // Usually 0xFFFFFFFF
    u64 buffer_mask;
    GObjScript gobjscripts[5];
    s32 gobjscripts_num;                // Length/number of active members of gobjlinks
    void *obj;                          // Can be: NULL, DObj, SObj or CObj
    f32 anim_frame;                     // Current frame of animation
    u32 flags;                          // GObj logic flags (e.g. 0x1 = skip rendering)
    void (*func_anim)(DObj*, s32, f32);
    GCUserData user_data;
};

struct GCTranslate
{
    XObj *xobj;

    union
    {
        Vec3f f;
        Vec3i i;

    } vec;
};

struct GCRotate
{
    XObj *xobj;

    f32 a;          // Rotation angle

    union
    {
        Vec3f f;

    } vec;
};

struct GCScale
{
    XObj *xobj;
    
    union
    {
        Vec3f f;

    } vec;
};

union AObjScript
{
    AObjEvent16 *event16;
    AObjEvent32 *event32;
};

struct DObj                 // Draw Object
{
    DObj *alloc_free;       // Has to do with memory allocation
    GObj *parent_gobj;      // GObj that this DObj belongs to
    DObj *sib_next;         // Next sibling
    DObj *sib_prev;         // Previous sibling
    DObj *child;            // Child
    DObj *parent;           // Parent

    GCTranslate translate;  // Translation XObj and vector
    GCRotate rotate;        // Rotation XObj and vector
    GCScale scale;          // Scale XObj and vector

    DObjVec *vec;

    union
    {
        void *dv;           // Default void* display list variable
        Gfx *dl;
        Gfx **dls;
        DObjMultiList *multi_list;
        DObjDLLink *dl_link;
        DObjDistDL *dist_dl;
        DObjDistDLLink *dist_dl_link;
    };

    u8 flags;
    
    ub8 is_anim_root;       // TRUE if this DObj's animation script is at the top of the hierarchy?

    u8 xobjs_num;
    XObj *xobjs[5];

    AObj *aobj;

    AObjScript anim_joint;
    
    f32 anim_wait;          // Multi-purpose? Usually frames remaining until next anim command, but used as rotation step in Crate/Barrel smash GFX?
    f32 anim_speed;         // Multi-purpose? Fighters use this as animation playback rate / interpolation, but it is used as rotation step in Crate/Barrel smash GFX?
    f32 anim_frame;         // Multi-purpose? Usually current animation frame, but used as rotation step in Crate/Barrel smash GFX?

    MObj *mobj;

    /* 
     * Can be:
     * Other DObj that this DObj is attached to
     * FTParts
     * YakumonoID
     * Color for 1P Game Master Hand defeat fadeout (?)
     */
    GCUserData user_data;
};

struct MPObjectColl
{
	f32 top;						// Diamond collision top
	f32 center;						// Diamond collision center
	f32 bottom;						// Diamond collision bottom
	f32 width;						// Diamond collision width
};

struct FTItemPickup
{
    Vec2f pickup_offset_light;
    Vec2f pickup_range_light;
    Vec2f pickup_offset_heavy;
    Vec2f pickup_range_heavy;
};

struct FTDamageCollDesc
{
    s32 joint_id;
    s32 placement;
    sb32 is_grabbable;
    Vec3f offset;
    Vec3f size;
};

struct FTAttributes
{
    f32 size;                                       // Model size
    f32 walkslow_anim_length;                       // Length of slow walk animation
    f32 walkmiddle_anim_length;                     // Length of medium walk animation
    f32 walkfast_anim_length;                       // Length of fast walk animation
    f32 throw_walkslow_anim_length;                 // Length of cargo slow walk animation
    f32 throw_walkmiddle_anim_length;               // Length of cargo medium walk animation
    f32 throw_walkfast_anim_length;                 // Length of cargo fast walk animation
    f32 rebound_anim_length;                        // Length of rebound animation
    f32 walk_speed_mul;
    f32 traction;
    f32 dash_speed;
    f32 dash_decel;
    f32 run_speed;
    f32 kneebend_anim_length;                       // Jump-squat frames
    f32 jump_vel_x;
    f32 jump_height_mul;
    f32 jump_height_base;
    f32 jumpaerial_vel_x;
    f32 jumpaerial_height;
    f32 air_accel;
    f32 air_speed_max_x;
    f32 air_friction;
    f32 gravity;
    f32 tvel_base;                                  // Normal terminal velocity
    f32 tvel_fast;                                  // Fast-fall terminal velocity
    s32 jumps_max;                                  // Number of jumps
    f32 weight;                                     // Or, rather, "knockback multiplier"
    f32 attack1_followup_frames;                    // Jab combo connection frames
    f32 dash_to_run;                                // Frames before dash transitions to run?
    f32 shield_size;                                // Size of fighter's shield (does not affect shield health)
    f32 shield_break_vel_y;                         // Y-Velocity of fighter's shield break
    f32 shadow_size;                                // Size of fighter's shadow
    f32 jostle_width;                               // ???
    f32 jostle_x;                                   // ???
    sb32 is_metallic;                               // Used to determine whether the character makes blue sparks or gray metal dust particles when hit
    f32 cam_offset_y;
    f32 closeup_camera_zoom;
    f32 camera_zoom;
    f32 camera_zoom_base;
    MPObjectColl map_coll;                          // Map collision diamond dimensions
    Vec2f cliffcatch_coll;                          // Ledge grab box dimensions
    u16 dead_fgm_ids[2];                            // KO voices
    u16 deadup_sfx;                                 // Star-KO voice
    u16 damage_sfx;
    u16 smash_sfx[3];                               // Random Smash SFX
    FTItemPickup item_pickup;
    u16 itemthrow_vel_scale;                        // Scale of thrown item velocity in %
    u16 itemthrow_damage_scale;                     // Scale of thrown item damage in %
    u16 heavyget_sfx;
    f32 halo_size;                                  // Respawn platform size?
    SYColorRGBA shade_color[3];
    SYColorRGBA fog_color;
    ub32 is_have_attack11    : 1;
    ub32 is_have_attack12    : 1;
    ub32 is_have_attackdash  : 1;
    ub32 is_have_attacks3    : 1;
    ub32 is_have_attackhi3   : 1;
    ub32 is_have_attacklw3   : 1;
    ub32 is_have_attacks4    : 1;
    ub32 is_have_attackhi4   : 1;
    ub32 is_have_attacklw4   : 1;
    ub32 is_have_attackairn  : 1;
    ub32 is_have_attackairf  : 1;
    ub32 is_have_attackairb  : 1;
    ub32 is_have_attackairhi : 1;
    ub32 is_have_attackairlw : 1;
    ub32 is_have_specialn    : 1;
    ub32 is_have_specialairn : 1;
    ub32 is_have_specialhi   : 1;
    ub32 is_have_specialairhi: 1;
    ub32 is_have_speciallw   : 1;
    ub32 is_have_specialairlw: 1;                   
    ub32 is_have_catch       : 1;                   // Whether fighter has a grab
    ub32 is_have_voice       : 1;                   // Whether fighter can play FGMs marked "voice"
    FTDamageCollDesc damage_coll_descs[11];         // Hurtbox array setup
    Vec3f hit_detect_range;                         // This is a radius around the fighter within which hitbox detection can occur
    u32 *setup_parts;                               // Pointer to two sets of flags marking joints that should be initialized on fighter creation
    u32 *animlock;                                  // Pointer to two sets of flags marking joints that should not be animated; ignores joints 0 through 3
    s32 effect_joint_ids[5];                        // The game will cycle through these joints when applying certain particles such as electricity and flames
    sb32 cliff_status_ga[5];                        // Bool for whether fighter is grounded or airborne during each cliff state
    s32 unused_0x2CC;                               // ???
    FTHiddenPart *hiddenparts;                      // Hidden fighter body parts?
    FTCommonPartContainer *commonparts_container;   // Base fighter body parts
    DObjDesc *dobj_lookup;                          // I don't actually know how this works at the moment
    AObjEvent32 **shield_anim_joints[8];            // One for each ordinal direction
    s32 joint_rfoot_id;                             // Right foot joint
    f32 joint_rfoot_rotate;                         // Amount of bend applied to right foot on slope contour?
    s32 joint_lfoot_id;                             // Left foot joint
    f32 joint_lfoot_rotate;                         // Amount of bend applied to left foot on slope contour?
    u8 filler_0x30C[0x31C - 0x30C];
    f32 unk_0x31C;
    f32 unk_0x320;
    Vec3f *translate_scales;                        // A set of scaling vectors to modify the translation vector of a given joint?
    FTModelPartContainer *modelparts_container;     // Passive model parts controlled via motion events or code
    FTAccessPart *accesspart;                       // Headgear accessory (Pikachu wizard hat or Jigglypuff bow)
    FTTexturePartContainer *textureparts_container; // These are generally facial expressions, controlled via motion events
    s32 joint_itemheavy_id;                         // Joint for holding heavy items
    FTThrownStatusArray *thrown_status;             // Array of thrown status IDs (forward- and back throw) to use for thrown fighters
    s32 joint_itemlight_id;                         // Joint for holding light items
    FTSprites *sprites;                             // Stock sprites, stock palettes and emblem sprites
    FTSkeleton **skeleton;                          // Electric damage skeleton model data
};

struct FTStruct
{
    FTStruct *next;                     // Next free FTStruct
    GObj *fighter_gobj;                 // Fighter's GObj
    s32 fkind;                          // Fighter ID
    u8 team;                            // Fighter's team
    u8 player;                          // Controller port assigned to this fighter
    u8 detail_curr;                     // Hi-Poly = 1, Low-Poly = 2
    u8 detail_base;                     // Hi-Poly = 1, Low-Poly = 2
    u8 costume;                         // Costume ID
    u8 shade;                           // i.e. When multiple instances of the same character costume are in-game
    u8 handicap;                        // Handicap value
    u8 level;                           // CPU player difficulty
    s8 stock_count;                     // Remaining stocks, runs out at -1 for some reason
    u8 team_order;                      // Order number if this fighter is a "VS *character* Team" member; used to check for bonuses such as Yoshi Rainbow
    u8 dl_link;                         // Rendering layer in which the fighter is drawn
    s32 player_num;                     // Player's number in the order they were created

    u32 status_total_tics;              // Tics spent in this action state

    s32 pkind;                          // Control type: 0 = HMN, 1 = CPU, 2 = N/A

    s32 status_id;                      // ID of current status
    s32 motion_id;                      // ID of motion script to use

    s32 percent_damage;                 // Fighter's damage
    s32 damage_resist;                  // Resits a specific amount of % damage before breaking, effectively damage-based armor
    s32 shield_health;                  // Current shield health
    f32 unk_ft_0x38;                    // Initialized to 0.0F, but never used
    s32 unk_ft_0x3C;                    // Initialized to 0, but never used
    u32 hitlag_tics;                    // Remaining hitlag tics
    s32 lr;                             // Facing direction; -1 = left, 0 = center, 1 = right

    struct FTPhysics
    {
        Vec3f vel_air;                  // Aerial self-induced velocity
        Vec3f vel_damage_air;           // Aerial knockback velocity
        Vec3f vel_ground;               // Grounded self-induced velocity
        f32 vel_damage_ground;          // Grounded knockback velocity
        f32 vel_jostle_x;               // X-Velocity from being pushed by another fighter
        f32 vel_jostle_z;               // Z-Velocity from being pushed by another fighter

    } physics;

    //MPCollData coll_data;               // Collision data

    u8 jumps_used;                      // Jumps used out of maximum available jumps
    u8 unk_ft_0x149;                    // ???
    sb32 ga;                            // Ground / Air bool

    f32 attack1_followup_frames;        // Frames of input window to get the next jab combo
    s32 attack1_status_id;              // Current jab status ID
    s32 attack1_input_count;            // Number of A presses required for rapid jab; 1-frame A presses count as 2 inputs
    s32 cliffcatch_wait;                // Wait this many frames before fighter can grab a ledge again
    s32 tics_since_last_z;              // Frames since last Z-press, resets to 65536 on action state change
    s32 acid_wait;                      // Wait this many frames before fighter can be hurt by Planet Zebes acid again
    s32 twister_wait;                   // Wait this many frames before fighter can be picked up by the Hyrule Tornado again
    s32 tarucann_wait;                  // Wait this many frames before fighter can enter Barrel Cannon again
    s32 damagefloor_wait;               // Wait this many frames before fighter can be hurt by damaging floors again (e.g. Mario's Board the Platforms stage)
    s32 playertag_wait;                 // Wait this many frames before fighter's player indicator is shown again; tag is shown when this reaches 1 or is overridden by position on stage

    s32 card_anim_frame_id;             // Index of fighter's role on 1P Stage Card scene? (e.g. player character, opponent, ally etc.)

    union FTCommandVars
    {
        // 
        struct FTCommandFlags
        {
            u32 flag0;
            u32 flag1;
            u32 flag2;
            u32 flag3;

        } flags;

        struct FTItemThrowFlags
        {
            sb32 is_throw_item;
            u8 unk1;
            u32 damage : 24;
            u8 unk2;
            u32 vel : 12;
            s32 angle : 12;

        } item_throw;

    } motion_vars;

    ub32 is_attack_active : 1;          // Whether fighter has active hitboxes
    ub32 is_hitstatus_nodamage : 1;     // Whether fighter's hurtbox state is impervious
    ub32 is_damage_coll_modify : 1;     // Whether fighter is using custom temporary hurtboxes
    ub32 is_modelpart_modify : 1;       // Whether fighter is using a specific non-default model part
    ub32 is_texturepart_modify : 1;     // Whether fighter is using a specific non-default face texture
    ub32 is_reflect : 1;                // Whether fighter's reflection collision sphere is active
    s32 reflect_lr : 2;                 // Direction of incoming hitbox subject to reflection
    ub32 is_absorb : 1;                 // Whether fighter's absorption collision sphere is active
    s32 absorb_lr : 2;                  // Direction of incoming hitbox subject to absorption
    ub32 is_goto_attack100 : 1;         // Whether rapid jab follow-up is confirmed
    ub32 is_fastfall : 1;               // Whether fighter is fast-falling
    ub32 is_magnify_show : 1;           // Whether fighter's magnifying glass is visible
    ub32 is_limit_map_bounds : 1;       // Effectively keeps the player at the blast zone and ignores the dead action state if TRUE
    ub32 is_invisible : 1;              // Whether fighter is invisible
    ub32 is_shadow_hide : 1;            // Whether fighter's shadow should be hidden
    ub32 is_rebirth : 1;                // Whether fighter is in any of the revive states after getting KO'd
    ub32 is_magnify_ignore : 1;         // Skip rendering magnifying glass if TRUE?
    ub32 is_playertag_hide : 1;         // Skip rendering player indicator if TRUE
    ub32 is_playertag_bossend : 1;      // Also skips rendering player indicator? Used only in "Master Hand defeated" cinematic from what I can tell so far
    ub32 is_effect_skip : 1;            // Toggled ON when fighter enters a pipe on Mushroom Kingdom; prevents particle script events from being executed
    u32 effect_joint_array_id : 4;      // Goes up to 5 by default; index of the array from effect_joint_ids from FTAttributes which houses the actual joint ID
    ub32 is_shield : 1;                 // Whether fighter's shield collision sphere is active
    ub32 is_effect_attach : 1;          // Whether fighter has an effect attached to one of their joints
    ub32 is_jostle_ignore : 1;          // Whether fighter is immune to jostling
    ub32 is_have_translate_scale : 1;   // Whether fighter is using translation vector transformation scales for animations
    ub32 is_control_disable : 1;        // Fighter cannot be controlled if TRUE; enabled when training mode menu is up
    ub32 is_hitstun : 1;                // Whether fighter is in hitstun
    u32 slope_contour : 3;              // "Slope Contour" behavior flags: 0x1 = left foot, 0x2 = right foot, 0x4 = full body rotation relative to slanted surfaces
    ub32 is_use_animlocks : 1;          // If TRUE, fighter will ignore animation of specified joints
    ub32 is_muted : 1;                  // This is set to TRUE only in the Characters menu; prevents motion script sound effects from playing
    ub32 unk_ft_0x190_b5 : 1;           // Unused
    ub32 is_item_show : 1;              // Whether fighter's item is visible
    ub32 is_cliff_hold : 1;             // Whether fighter is holding onto a ledge
    ub32 is_events_forward : 1;         // Is this flag's sole purpose to fast-forward GFX in the moveset event parser?
    ub32 is_ghost : 1;                  // If TRUE, fighter does not check for hit collisions and cannot be KO'd
    ub32 is_damage_resist : 1;          // Whether fighter's damage-based armor is active
    ub32 is_menu_ignore : 1;            // Fighter cannot bring up training or pause menu if TRUE
    u32 camera_mode : 4;                // Camera behavior for this fighter
    ub32 is_special_interrupt : 1;      // Whether move can be interrupted by Link's boomerang? Have not seen this used anywhere else
    ub32 is_ignore_dead : 1;            // Ignore dead action states altogether
    ub32 is_catchstatus : 1;            // Whether fighter's current state is marked as a grab
    ub32 is_catch_or_capture : 1;       // My brain stops working everytime I try to understand what this does
    ub32 is_use_fogcolor : 1;           // Whether fighter is using fog-based color overlay
    ub32 is_shield_catch : 1;           // Set to TRUE when fighter grabs after getting shield poked; there is a check for this flag that halves throw damage if TRUE
    ub32 is_knockback_paused : 1;       // Whether fighter's knockback is paused until hitlag is over?

    u8 capture_immune_mask;             // Fighter is immune to these grab types
    u8 catch_mask;                      // Fighter's current grab type

    //FTAnimDesc anim_desc;               // Fighter's animation flags
    Vec3f anim_vel;                     // Fighter's animation-induced velocity

    Vec2f magnify_pos;                  // Fighter's magnifying glass position

    // struct FTInputStruct
    // {
    //     SYController *controller;       // Controller inputs
    //     u16 button_mask_a;              // Button mapped to N64 A-Button
    //     u16 button_mask_b;              // Button mapped to N64 B-Button
    //     u16 button_mask_z;              // Button mapped to N64 Z-Trigger
    //     u16 button_mask_l;              // Button mapped to N64 L-Trigger
    //     FTPlayerInput pl;               // Human player inputs
    //     FTComputerInput cp;             // Computer player inputs

    // } input;

    // FTComputer computer;                // Computer player struct

    Vec2f damage_coll_size;             // Width and height of fighter's hurtbox; calculated from distance of TopN position to farthest hurtbox multiplied by 0.55

    u8 tap_stick_x;                     // Frames control stick has been tapped
    u8 tap_stick_y;                     // Frames control stick has been tapped
    u8 hold_stick_x;                    // Frames control stick has been tapped or held
    u8 hold_stick_y;                    // Frames control stick has been tapped or held

    s32 breakout_wait;                  // Frames until fighter breaks out of shield break / sleep / Cargo Throw
    s8 breakout_lr;                     // Whether victim is mashing left or right
    s8 breakout_ud;                     // Whether victim is mashing up or down

    u8 shuffle_frame_index;             // Ranges from 0-3; position of fighter's model vibration is adjusted based on this index when receiving hitlag
    u8 shuffle_index_max;               // How many iterations the frame index increments before looping back to 0;
    ub8 is_shuffle_electric;            // Fighter vibrates horizontally rather than vertically if hit by an electric attack
    u16 shuffle_tics;                   // Model shift timer

    GObj *throw_gobj;                   // GObj of opponent that threw this fighter
    s32 throw_fkind;                    // Kind of opponent that threw this fighter
    u8 throw_team;                      // Team of opponent that threw this fighter
    u8 throw_player;                    // Port of opponent that threw this fighter
    s32 throw_player_num;               // Player number of opponent that threw this fighter

    u32 motion_attack_id;               // Used to group status_ids for the stale queue
    u16 motion_count;                   // Multihits do not stale if motion_attack_id and motion_count are equal to the previous staling queue entry
    // GMStatFlags stat_flags;             // Move statistics for determining what 1P Game bonus stats are awarded
    u16 stat_count;                     // Used to keep track of attacks for 1P Game bonus stats

    // FTAttackColl attack_colls[4];       // Fighter's hitboxes

    s32 invincible_tics;                // Remaining tics of invincibility
    s32 intangible_tics;                // Remaining tics of intangibility
    s32 special_hitstatus;              // Type of custom, timed hit collision state
    s32 star_invincible_tics;           // Remaining tics of Starman invincibility
    s32 star_hitstatus;                 // Type of Starman hit collision state
    s32 hitstatus;                      // Type of passive hit collision state

    // FTDamageColl damage_colls[11];

    f32 unk_ft_0x7A0;                   // Unused
    f32 hitlag_mul;                     // Hitlag multiplier; 1.5x for both attacker and defender if the connecting hitbox's element is electric
    f32 shield_heal_wait;               // How long before fighter's shield health recovers by 1 unit
    s32 unk_ft_0x7AC;                   // Unused

    s32 attack_damage;
    f32 attack_knockback;
    u16 attack_count;                   // Number of times this fighter successfully dealt damage 
    s32 attack_shield_push;             // Used to calculate shield/rebound pushback
    f32 attack_rebound;                 // Actually 2x staled damage?
    s32 hit_lr;
    s32 shield_damage;
    s32 shield_damage_total;            // shield_damage + hitbox damage + hitbox shield damage, does not persist?
    s32 shield_lr;                      // Direction in which shield knockback is set to occur
    s32 shield_player;                  // Port of player hitting this fighter's shield
    s32 reflect_damage;
    s32 damage_lag;                     // Used to calculate hitlag?
    f32 damage_knockback;
    f32 knockback_resist_passive;       // Passive armor, always active (?)
    f32 knockback_resist_status;        // Resist this many units of knockback, effectively knockback-based armor
    f32 damage_knockback_stack;         // Knockback stacking?
    s32 damage_queue;                   // Used to calculate knockback?
    s32 damage_angle;
    s32 damage_element;
    s32 damage_lr;
    s32 damage_index;
    s32 damage_joint_id;
    s32 damage_player_num;
    s32 damage_player;                  // Port index of damaging fighter
    u16 damage_count;                   // Number of times fighter has taken damage; initialized to 0 and incremented, but never used?
    s32 damage_kind;
    s32 damage_heal;                    // Percent damage to heal
    f32 damage_mul;
    s32 damage_object_class;            // Fighter, Weapon, Item or Ground
    s32 damage_object_kind;             // FTKind, WPKind, ITKind, envKind
    // GMStatFlags damage_stat_flags;
    u16 damage_stat_count;

    f32 public_knockback;               // Knockback value used for crowd reactions

    GObj *search_gobj;                  // GObj this fighter found when searching for grabbable fighters
    f32 search_gobj_dist;               // Distance to found fighter GObj
    void (*proc_catch)(GObj*);          // Run this callback on grabbing attacker
    void (*proc_capture)(GObj*, GObj*); // Run this callback on grabbed victim
    GObj *catch_gobj;                   // GObj this fighter has caught
    GObj *capture_gobj;                 // GObj this fighter is captured by

    FTThrowHitDesc *throw_desc;         // Pointer to throw description

    GObj *item_gobj;                    // Item GObj this fighter is holding

    FTSpecialColl *special_coll;        // Fighter's special collision (This is a sphere! Not a box!)

    Vec3f entry_pos;                    // Fighter's initial spawn position

    f32 camera_zoom_frame;              // Maximum size of fighter's camera range?
    f32 camera_zoom_range;              // Multiplier of fighter's camera range?

    // FTMotionScript motion_scripts[2][2];// Fighter's move scripts; [i][0] is played before, then [i][1] once all 'proc_whatevers' have been executed 

    DObj *joints[FTPARTS_JOINT_NUM_MAX];// Fighter's joints (DObjs)

    // -1 = hidden, 0 and up = draw model part ID
    // FTModelPartStatus modelpart_status[FTPARTS_JOINT_NUM_MAX - nFTPartsJointCommonStart];

    // FTTexturePartStatus texturepart_status[2];

    FTData *data;                   // Fighter's file data setup
    FTAttributes *attr;             // Fighter's unique attributes

    void **figatree;                // Main animation
    void **figatree_heap;           // Extern heap to load animations into

    void (*proc_update)(GObj*);     // Update process
    void (*proc_accessory)(GObj*);  // Runs at the beginning of status change if events aren't queued and whenever fighter isn't in hitlag
    void (*proc_interrupt)(GObj*);  // Interrupt process
    void (*proc_physics)(GObj*);    // Physics process
    void (*proc_map)(GObj*);        // Map collisions process
    void (*proc_slope)(GObj*);      // Slope Contour process
    void (*proc_damage)(GObj*);     // Runs when fighter is damaged
    void (*proc_trap)(GObj*);       // Used only by Yoshi Egg?
    void (*proc_shield)(GObj*);     // Runs when hitbox contact with a shield is made
    void (*proc_hit)(GObj*);        // Runs when hitbox contact with a hurtbox is made
    void (*proc_passive)(GObj*);    // Runs regardless of hitlag during the Update / Interrupt main process
    void (*proc_lagupdate)(GObj*);  // Runs on every tic when fighter is in hitlag
    void (*proc_lagstart)(GObj*);   // Runs when fighter enters hitlag
    void (*proc_lagend)(GObj*);     // Runs when fighter's hitlag is over
    void (*proc_status)(GObj*);     // Runs only on status change

    // alSoundEffect *p_sfx;
    // u16 sfx_id;
    // alSoundEffect *p_voice;
    // u16 voice_id;
    // alSoundEffect *p_loop_sfx;
    // u16 loop_sfx_id;

    // GMColAnim colanim;

    SYColorRGBA fog_color;          // Used only by Master Hand, when in the background on the -Z plane?
    SYColorRGBA shade_color;        // Shade colors of character costume

    // FTKey key;                      // Automatic input sequence struct

    // struct FTAfterImageInfo
    // {
    //     ub8 is_itemswing;
    //     s8 drawstatus;
    //     u8 desc_id;
    //     FTAfterImage desc[3];

    // } afterimage;

    // Variables independent of current status ID
    // union FTPassiveVars
    // {
    //     FTMarioPassiveVars      mario;
    //     FTDonkeyPassiveVars     donkey;
    //     FTSamusPassiveVars      samus;
    //     FTLinkPassiveVars       link;
    //     FTCaptainPassiveVars    captain;
    //     FTKirbyPassiveVars      kirby;
    //     FTPikachuPassiveVars    pikachu;
    //     FTPurinPassiveVars      purin;
    //     FTNessPassiveVars       ness;
    //     FTBossPassiveVars       boss;

    // } passive_vars;

    s32 hammer_tics;

    // Variables exclusive to current status ID or groups of status IDs
    // union FTStatusVars
    // {
    //     FTCommonStatusVars      common;
    //     FTMarioStatusVars       mario;
    //     FTFoxStatusVars         fox;
    //     FTDonkeyStatusVars      donkey;
    //     FTSamusStatusVars       samus;
    //     FTLinkStatusVars        link;
    //     FTYoshiStatusVars       yoshi;
    //     FTCaptainStatusVars     captain;
    //     FTKirbyStatusVars       kirby;
    //     FTPikachuStatusVars     pikachu;
    //     FTNessStatusVars        ness;
    //     FTBossStatusVars        boss;

    // } status_vars;

    s32 display_mode;
};

struct FTCommonPart
{
    DObjDesc *dobjdesc;
    MObjSub ***p_mobjsubs;
    AObjEvent32 ***p_costume_matanim_joints;
    u8 flags;
};

struct FTCommonPartContainer
{
    FTCommonPart commonparts[2];
};

struct DObjDesc
{
    s32 id;
    void *dl;
    Vec3f translate;
    Vec3f rotate;
    Vec3f scale;
};

struct MObjSub
{
    u16 pad00;
    u8 fmt;
    u8 siz;
    void **sprites; // should this be a pointer to an array of images (sprite set)?
    u16 unk08;
    u16 unk0A;
    u16 unk0C;
    u16 unk0E;
    s32 unk10; // could be f32??
    f32 trau;       // U-Translation?
    f32 trav;       // V-Translation?
    f32 scau;       // U-Scale?
    f32 scav;       // V-Scale?
    f32 unk24;
    f32 unk28;
    void **palettes;  // palette pointers?
    u16 flags;      // command flags?
    u8 block_fmt;   // texture image format?
    u8 block_siz;
    u16 block_dxt;
    u16 unk36;
    u16 unk38;
    u16 unk3A;
    f32 scrollu;
    f32 scrollv;
    f32 unk44;
    f32 unk48;       // Translate V?
    u32 unk4C;
    SYColorPack primcolor;
    u8 prim_l;
    u8 prim_m;
    u8 prim_pad[2];
    SYColorPack envcolor;
    SYColorPack blendcolor;
    SYColorPack light1color;
    SYColorPack light2color;
    s32 unk68;
    s32 unk6C;
    s32 unk70;
    s32 unk74;
};

struct MObj                         // Material Object
{
    MObj *next;
    GObj *parent_gobj;              // Unconfirmed
    MObjSub sub;
    u16 texture_id_curr;
    u16 texture_id_next;
    f32 lfrac;
    f32 palette_id;
    s32 unk_mobj_0x8C;
    AObj *aobj;
    AObjScript matanim_joint;
    f32 anim_wait;                  // Animation frames remaining until next command(s) are parsed
    f32 anim_speed;                 // Animation playback rate / interpolation, multi-purpose?
    f32 anim_frame;                 // Current animation frame, multi-purpose?
	GCUserData user_data;           // Actually just padding?
};

struct XObj 
{
    XObj *next;
    u8 kind;
    u8 unk05;
    Mtx mtx;
};

struct DObjVec
{
    u8 kinds[3];
    u8 pad;
    u8 data[1];
};