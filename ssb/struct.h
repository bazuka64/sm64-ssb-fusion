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