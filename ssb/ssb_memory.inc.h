void lbRelocLoadAndRelocFile(u32 file_id, void *ram_dst, u32 bytes_num, s32 loc);

void lbRelocInitSetup(LBRelocSetup* setup)
{
    sLBRelocInternBuffer.rom_table_lo = setup->table_addr;
    sLBRelocInternBuffer.total_files_num = setup->table_files_num;
    sLBRelocInternBuffer.rom_table_hi = setup->table_addr + ((setup->table_files_num + 1) * sizeof(LBTableEntry));

    sLBRelocInternBuffer.heap_start = sLBRelocInternBuffer.heap_ptr = setup->file_heap;
    sLBRelocInternBuffer.heap_end = (void*)((uintptr_t)setup->file_heap + setup->file_heap_size);

    sLBRelocCurrentTableEntry = (void*)LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocTableEntries);
    sLBRelocNextTableEntry = sLBRelocCurrentTableEntry + 1;

    sLBRelocInternBuffer.status_buffer_num = 0;
    sLBRelocInternBuffer.status_buffer_max = setup->status_buffer_size;
    sLBRelocInternBuffer.status_buffer = setup->status_buffer;

    sLBRelocInternBuffer.force_status_buffer_max = setup->force_status_buffer_size;
    sLBRelocInternBuffer.force_status_buffer = setup->force_status_buffer;
}

void scVSBattleSetupFiles() {
    LBRelocSetup rl_setup;

    rl_setup.table_addr = 0x00800000;
    rl_setup.table_files_num = 2132;
    rl_setup.file_heap = NULL;
    rl_setup.file_heap_size = 0;
    rl_setup.status_buffer = sSCVSBattleStatusBuffer;
    rl_setup.status_buffer_size = ARRAY_COUNT(sSCVSBattleStatusBuffer);
    rl_setup.force_status_buffer = sSCVSBattleForceStatusBuffer;
    rl_setup.force_status_buffer_size = ARRAY_COUNT(sSCVSBattleForceStatusBuffer);

    lbRelocInitSetup(&rl_setup);
}

void* syMallocSet(SYMallocRegion* bp, size_t size, u32 alignment)
{
    u8* aligned;
    u32 offset;

    if (alignment != 0)
    {
        offset = alignment - 1;
        aligned = (u8*)(((uintptr_t)bp->ptr + (offset)) & ~(offset));
    }
    else aligned = (u8*)bp->ptr;

    bp->ptr = (void*)(aligned + size);

    if (bp->end < bp->ptr)
    {
        syDebugPrintf("ml : alloc overflow #%d\n", bp->id);

        while (TRUE);
    }
    return (void*)aligned;
}

void syMallocInit(SYMallocRegion* bp, u32 id, void* start, size_t size)
{
    bp->id = id;
    bp->ptr = start;
    bp->start = start;
    bp->end = (void*)((uintptr_t)start + size);
}

void* lbRelocFindStatusBufferFile(u32 id)
{
    s32 i;

    if (sLBRelocInternBuffer.status_buffer_num == 0)
    {
        return NULL;
    }
    else for (i = 0; i < sLBRelocInternBuffer.status_buffer_num; i++)
    {
        if (id == sLBRelocInternBuffer.status_buffer[i].id)
        {
            return sLBRelocInternBuffer.status_buffer[i].addr;
        }
    }
    return NULL;
}

void syDmaReadRom(uintptr_t rom_src, void *ram_dst, size_t size) 
{
    dma_read((u8*)ram_dst, (u8*)rom_src, (u8*)(rom_src + size));
}

void lbRelocReadDmaTableEntry(u32 entry_id)
{
    syDmaReadRom
    (
        sLBRelocInternBuffer.rom_table_lo + (entry_id * sizeof(LBTableEntry)),
        sLBRelocCurrentTableEntry,
        sizeof(LBTableEntry) * 2
    );
}

size_t lbRelocGetExternBytesNum(u32 file_id)
{
    u16 *rom_extern_csr;
    u16 *file_id_read;
    size_t bytes_read;
    u16 *end;
    size_t compressed_size;
    void *rom_end;
    s32 unused;
    u8 file_ids_buf[16];
    s32 i;

    if (lbRelocFindStatusBufferFile(file_id) != NULL)
    {
        return 0;
    }
    else for (i = 0; i < sLBRelocExternFileIDsNum; i++)
    {
        if (file_id == sLBRelocExternFileIDs[i])
        {
            return 0;
        }
    }
    if (sLBRelocExternFileIDsNum >= sLBRelocExternFileIDsMax)
    {
        while (TRUE)
        {
            syDebugPrintf("Relocatable Data Manager: External Data is over %d!!\n", sLBRelocExternFileIDsMax);
            scManagerRunPrintGObjStatus();
        }
    }
    lbRelocReadDmaTableEntry(file_id);
    
    bytes_read = (u32) LBRELOC_CACHE_ALIGN(sLBRelocCurrentTableEntry->decompressed_size * sizeof(u32));
    compressed_size = sLBRelocCurrentTableEntry->compressed_size * sizeof(u32);
        
    sLBRelocExternFileIDs[sLBRelocExternFileIDsNum++] = file_id;
        
    rom_end = (void*) (sLBRelocInternBuffer.rom_table_hi + sLBRelocCurrentTableEntry->data_offset); // Ew...

    rom_extern_csr = (u16*) ((uintptr_t)rom_end + compressed_size);
    
    end = (u16*) (sLBRelocInternBuffer.rom_table_hi + sLBRelocNextTableEntry->data_offset);

    file_id_read = (void*) LBRELOC_CACHE_ALIGN((uintptr_t)file_ids_buf);
    
    while (rom_extern_csr < end)
    {
        syDmaReadRom((uintptr_t)rom_extern_csr, file_id_read, sizeof(u16));
        bytes_read += lbRelocGetExternBytesNum(*file_id_read);
        rom_extern_csr++;
    }
    return bytes_read;
}

size_t lbRelocGetFileSize(u32 id)
{
    u32 file_ids_extern_buf[50];

    sLBRelocExternFileIDs = file_ids_extern_buf;
    sLBRelocExternFileIDsNum = 0;
    sLBRelocExternFileIDsMax = ARRAY_COUNT(file_ids_extern_buf);

    return lbRelocGetExternBytesNum(id);
}

void* syTaskmanMalloc(size_t size, u32 align) // alloc_with_alignment
{
	return syMallocSet(&gSYTaskmanGeneralHeap, size, align);
}

void syDmaDecodeVpk0(u16 *data, size_t size, void (*update_stream)(void), u8 *out_buf)
{
#define VPK0_UPDATE_STREAM()        \
    if ((uintptr_t) csr >= bound)   \
    {                               \
        update_stream();            \
        csr = data;                 \
    }

#define VPK0_READ_USHORT()  \
    temp_value <<= 0x10;    \
    temp_value |= *(csr++); \
    bits_num += 0x10;

#define VPK0_GET_BITS(var, n) \
    if (bits_num < n) {       \
        VPK0_UPDATE_STREAM(); \
        VPK0_READ_USHORT();   \
    }                         \
    bits_num -= n;            \
    var = ((temp_value << ((32 - n) - bits_num)) >> (32 - (u32) (n)));

#define VPK0_INIT_NODE(node)    \
    node = lengths_node;        \
    lengths_node->left = NULL;  \
    lengths_node->right = NULL; \
    lengths_node->value = 0;    \
    lengths_node++;

    uintptr_t bound = (uintptr_t) ((uintptr_t)data + size);
    SYHuffmanNode *sample1_node;
    SYHuffmanNode *lengths_node;
    SYHuffmanNode sp14C[64];
    u8* out_ptr;
    SYHuffmanNode *offsets_tree;
    SYHuffmanNode *lengths_tree;
    u8* copy_src;
    void* out_buf_end;
    u32 sample_method;
    SYHuffmanNode *off_stack[20];
    s32 off_stack_size;
    s32 value;
    SYHuffmanNode *offsets_node;
    SYHuffmanNode *off_node;
    SYHuffmanNode *lengths_stack[20];
    s32 lengths_stack_size;
    s32 unused2[2];
    SYHuffmanNode *length_node;
    s32 unused3[3];
    s32 sp64;
    s32 unused;
    u32 temp_value;
    s32 bits_num;
    u16* csr;

    bits_num = 0;
    temp_value = 0;
    lengths_node = sp14C;

    // read "vpk0" magic
    update_stream();

    csr = data;
    // clang-format off
    VPK0_READ_USHORT(); VPK0_UPDATE_STREAM(); VPK0_READ_USHORT(); bits_num -= 32;
    // clang-format on

    out_ptr = out_buf;

    // read size of decompressed data
    // clang-format off
    VPK0_UPDATE_STREAM(); VPK0_READ_USHORT(); VPK0_UPDATE_STREAM(); VPK0_READ_USHORT(); \
    bits_num -= 32; out_buf_end = ((temp_value << ((32 - 32) - bits_num)) >> (32 - (u32) (32))) + out_buf;
    // clang-format on

    // read sample method
    VPK0_GET_BITS(sample_method, 8);

    // read Huffman tree for offsets
    off_stack_size = 0;
    off_stack[0] = NULL;

    while (TRUE)
    {
        // leaf or node
        VPK0_GET_BITS(value, 1);

        // node, but less than 2 nodes on stack -> end of tree
        if ((value != 0) && (off_stack_size < 2))
        {
            break;
        }
        if (value != 0)
        {
            // node
            VPK0_INIT_NODE(off_node);

            // combine two nodes from stack
            off_node->left = off_stack[off_stack_size - 2];
            off_node->right = off_stack[off_stack_size - 1];

            // write current node on stack
            off_stack[off_stack_size - 2] = off_node;
            off_stack_size--;
        }
        else
        {
            // leaf
            VPK0_INIT_NODE(off_node);

            // read leaf node value
            VPK0_GET_BITS(off_node->value, 8);

            off_stack[off_stack_size] = off_node;
            off_stack_size++;
        }
    }
    offsets_tree = off_stack[0];

    // read Huffman tree for lengths
    lengths_stack_size = 0;
    lengths_stack[0] = 0;

    while (TRUE)
    {
        // leaf or node
        VPK0_GET_BITS(value, 1);

        // node, but less than 2 nodes on stack -> end of tree
        if ((value != 0) && (lengths_stack_size < 2))
        {
            break;
        }
        if (value != 0)
        {
            // node
            VPK0_INIT_NODE(length_node);

            // combine two nodes from stack
            length_node->left = lengths_stack[lengths_stack_size - 2];
            length_node->right = lengths_stack[lengths_stack_size - 1];

            // write current node on stack
            lengths_stack[lengths_stack_size - 2] = length_node;
            lengths_stack_size--;
        }
        else
        {
            // leaf
            VPK0_INIT_NODE(length_node);

            // read leaf node value
            VPK0_GET_BITS(length_node->value, 8);

            lengths_stack[lengths_stack_size] = length_node;
            lengths_stack_size++;
        }
    }
    lengths_tree = lengths_stack[0];

    while ((uintptr_t)out_ptr < (uintptr_t)out_buf_end)
    {
        VPK0_GET_BITS(value, 1);

        if (!value)
        {
            // byte value
            VPK0_GET_BITS(*out_ptr++, 8);
        }
        else
        {
            // encoded data
            lengths_node = lengths_tree;

            if (sample_method != 0)
            {
                // two samples
                sp64 = 0;
                sample1_node = offsets_tree;

                while (sample1_node->left != NULL)
                {
                    VPK0_GET_BITS(value, 1);
                    sample1_node = !value ? sample1_node->left : sample1_node->right;
                }
                VPK0_GET_BITS(value, sample1_node->value);

                if (value <= 2)
                {
                    sp64 = value + 1;
                    offsets_node = offsets_tree;

                    while (offsets_node->left != NULL)
                    {
                        VPK0_GET_BITS(value, 1);
                        offsets_node = !value ? offsets_node->left : offsets_node->right;
                    }
                    VPK0_GET_BITS(value, offsets_node->value);
                }
                copy_src = out_ptr - value * 4 - sp64 + 8;
            }
            else
            {
                // one sample
                offsets_node = offsets_tree;
                // get number of bytes to move back
                while (offsets_node->left != NULL)
                {
                    VPK0_GET_BITS(value, 1);
                    offsets_node = !value ? offsets_node->left : offsets_node->right;
                }
                VPK0_GET_BITS(value, offsets_node->value);
                // move back
                copy_src = out_ptr - value;
            }
            // get number of bytes to copy
            while (lengths_node->left != NULL)
            {
                VPK0_GET_BITS(value, 1);
                lengths_node = !value ? lengths_node->left : lengths_node->right;
            }
            VPK0_GET_BITS(value, lengths_node->value);

            while (value-- > 0)
            {
                *(out_ptr++) = *(copy_src++);
            }
        }
    }
}

void syDmaInitVpk0Stream(uintptr_t dev_addr, void *ram_addr, size_t size)
{
    sSYDmaVpkDevAddr = dev_addr;
    sSYDmaVpkBuffer = ram_addr;
    sSYDmaVpkBufferSize = size;
}

void syDmaFillVpk0Buf(void)
{
    syDmaReadRom(sSYDmaVpkDevAddr, sSYDmaVpkBuffer, sSYDmaVpkBufferSize);
    sSYDmaVpkDevAddr += sSYDmaVpkBufferSize;
}

void syDmaReadVpk0Buf(uintptr_t dev_addr, void *ram_dst, void *ram_addr, size_t size)
{
    syDmaInitVpk0Stream(dev_addr, ram_addr, size);
    syDmaDecodeVpk0(ram_addr, size, syDmaFillVpk0Buf, ram_dst);
}

void syDmaReadVpk0(uintptr_t dev_addr, void *ram_dst)
{
    u16 buf[0x200];

    syDmaReadVpk0Buf(dev_addr, ram_dst, buf, sizeof(buf));
}

void lbRelocAddStatusBufferFile(u32 id, void *addr)
{
    u32 num = sLBRelocInternBuffer.status_buffer_num;

    if (num >= sLBRelocInternBuffer.status_buffer_max)
    {
        while (TRUE)
        {
            syDebugPrintf("Relocatable Data Manager: Status Buffer is full !!\n");
            scManagerRunPrintGObjStatus();
        }
    }
    sLBRelocInternBuffer.status_buffer[num].id = id;
    sLBRelocInternBuffer.status_buffer[num].addr = addr;
    sLBRelocInternBuffer.status_buffer_num++;
}

void lbRelocAddForceStatusBufferFile(u32 id, void *addr)
{
    u32 num = sLBRelocInternBuffer.force_status_buffer_num;

    if (num >= sLBRelocInternBuffer.force_status_buffer_max)
    {
        while (TRUE)
        {
            syDebugPrintf("Relocatable Data Manager: Force Status Buffer is full !!\n");
            scManagerRunPrintGObjStatus();
        }
    }
    sLBRelocInternBuffer.force_status_buffer[num].id = id;
    sLBRelocInternBuffer.force_status_buffer[num].addr = addr;
    sLBRelocInternBuffer.force_status_buffer_num++;
}

void* lbRelocGetInternBufferFile(u32 id)
{
    size_t file_size;
    void *file;
    void *file_alloc, *file_alloc_end, *heap_end;

    file = lbRelocFindStatusBufferFile(id);
    
    if (file != NULL)
    {
        return file;
    }
    file_alloc = (void*) LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocInternBuffer.heap_ptr);
    lbRelocReadDmaTableEntry(id);

    file_size = sLBRelocCurrentTableEntry->decompressed_size * sizeof(u32);
    
    if (((uintptr_t)file_alloc + file_size) > (uintptr_t)sLBRelocInternBuffer.heap_end)
    {
        while (TRUE)
        {
            syDebugPrintf("Relocatable Data Manager: Buffer is full !!\n");
            scManagerRunPrintGObjStatus();
        }
    }
    sLBRelocInternBuffer.heap_ptr = ((uintptr_t)file_alloc + file_size);
    
    lbRelocLoadAndRelocFile(id, file_alloc, file_size, nLBFileLocationDefault);

    return file_alloc;
}

void* lbRelocFindForceStatusBufferFile(u32 id)
{
    s32 i;

    if (sLBRelocInternBuffer.force_status_buffer_num != 0)
    {
        for (i = 0; i < sLBRelocInternBuffer.force_status_buffer_num; i++)
        {
            if (id == sLBRelocInternBuffer.force_status_buffer[i].id)
            {
                return sLBRelocInternBuffer.force_status_buffer[i].addr;
            }
        }
    }
    return lbRelocFindStatusBufferFile(id);
}

void* lbRelocGetForceExternBufferFile(u32 id)
{
    void *file_alloc;
    void *file;
    size_t file_size;

    file = lbRelocFindForceStatusBufferFile(id);

    if (file != NULL)
    {
        return file;
    }
    file_alloc = (void*) LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocExternFileHeap);
    lbRelocReadDmaTableEntry(id);

    file_size = sLBRelocCurrentTableEntry->decompressed_size * sizeof(u32);
    sLBRelocExternFileHeap = (void*) ((uintptr_t)file_alloc + file_size);

    lbRelocLoadAndRelocFile(id, file_alloc, file_size, nLBFileLocationForce);

    return file_alloc;
}

void* lbRelocGetExternBufferFile(u32 id)
{
    void *file_alloc;
    void *file;
    size_t file_size;

    file = lbRelocFindStatusBufferFile(id);

    if (file != NULL)
    {
        return file;
    }
    file_alloc = (void*) LBRELOC_CACHE_ALIGN((uintptr_t)sLBRelocExternFileHeap);
    lbRelocReadDmaTableEntry(id);

    file_size = sLBRelocCurrentTableEntry->decompressed_size * sizeof(u32);
    sLBRelocExternFileHeap = (void*) ((uintptr_t)file_alloc + file_size);

    lbRelocLoadAndRelocFile(id, file_alloc, file_size, nLBFileLocationExtern);

    return file_alloc;
}

void lbRelocLoadAndRelocFile(u32 file_id, void *ram_dst, u32 bytes_num, s32 loc)
{
    u16 *file_id_extern;
    LBRelocDesc *intern_desc, *extern_desc;
    void *vaddr_extern;
    u16 reloc_intern, reloc_extern;
    u16 file_ids[8];
    uintptr_t data_rom_offset;

    data_rom_offset = sLBRelocInternBuffer.rom_table_hi + sLBRelocCurrentTableEntry->data_offset;

    if (sLBRelocCurrentTableEntry->is_compressed)
    {
        syDmaReadVpk0(data_rom_offset, ram_dst);
    }
    else syDmaReadRom(data_rom_offset, ram_dst, bytes_num);

    if (loc == nLBFileLocationForce)
    {
        lbRelocAddForceStatusBufferFile(file_id, ram_dst);
    }
    else lbRelocAddStatusBufferFile(file_id, ram_dst);
    
    reloc_intern = sLBRelocCurrentTableEntry->reloc_intern_offset;
    
    while (reloc_intern != 0xFFFF)
    {
        intern_desc = (LBRelocDesc*) ((uintptr_t)ram_dst + (reloc_intern * sizeof(u32)));
        reloc_intern = intern_desc->info.reloc;
        intern_desc->p = (void*) ((intern_desc->info.words_num * sizeof(u32)) + (uintptr_t)ram_dst);
    }
    reloc_extern = sLBRelocCurrentTableEntry->reloc_extern_offset;
    
    data_rom_offset += sLBRelocCurrentTableEntry->compressed_size * sizeof(u32);
    
    file_id_extern = (void*) LBRELOC_CACHE_ALIGN((uintptr_t)file_ids);
    
    while (reloc_extern != 0xFFFF)
    {
        extern_desc = (LBRelocDesc*) ((uintptr_t)ram_dst + (reloc_extern * sizeof(u32)));
        reloc_extern = extern_desc->info.reloc;

        syDmaReadRom(data_rom_offset, file_id_extern, sizeof(u16));
        
        if (loc == nLBFileLocationForce)
        {
            vaddr_extern = lbRelocFindForceStatusBufferFile(*file_id_extern);
        }
        else vaddr_extern = lbRelocFindStatusBufferFile(*file_id_extern);

        if (vaddr_extern == NULL)
        {
            switch (loc)
            {
            case nLBFileLocationExtern:
                vaddr_extern = lbRelocGetExternBufferFile(*file_id_extern);
                break;
                
            case nLBFileLocationDefault:
                vaddr_extern = lbRelocGetInternBufferFile(*file_id_extern);
                break;
                
            case nLBFileLocationForce:
                vaddr_extern = lbRelocGetForceExternBufferFile(*file_id_extern);
                break;
            }
        }
        extern_desc->p = (void*)((extern_desc->info.words_num * sizeof(u32)) + (uintptr_t)vaddr_extern);
        data_rom_offset += sizeof(u16);

        continue;
    }
}

void* lbRelocGetExternHeapFile(u32 id, void *heap)
{
    sLBRelocExternFileHeap = heap;
    return lbRelocGetExternBufferFile(id);
}

void ftManagerSetupFilesMainKind(s32 fkind)
{
    FTData *data = &dFTNessData;

    *data->p_file_main = lbRelocGetExternHeapFile(data->file_main_id, syTaskmanMalloc(lbRelocGetFileSize(data->file_main_id), 0x10));
}

void* lbRelocGetStatusBufferFile(u32 id)
{
    return lbRelocFindStatusBufferFile(id);
}

void ftManagerSetupFilesKind(s32 fkind)
{
    FTData *data = &dFTNessData;

    if (data->file_mainmotion_id != 0)
    {
        *data->p_file_mainmotion = lbRelocGetStatusBufferFile(data->file_mainmotion_id);
    }
    if (data->file_submotion_id != 0)
    {
        *data->p_file_submotion = lbRelocGetStatusBufferFile(data->file_submotion_id);
    }
    *data->p_file_model = lbRelocGetStatusBufferFile(data->file_model_id);

    if (data->file_shieldpose_id != 0)
    {
        data->p_file_shieldpose = lbRelocGetStatusBufferFile(data->file_shieldpose_id);
    }
    if (data->file_special1_id != 0)
    {
        *data->p_file_special1 = lbRelocGetStatusBufferFile(data->file_special1_id);
    }
    if (data->file_special2_id != 0)
    {
        *data->p_file_special2 = lbRelocGetStatusBufferFile(data->file_special2_id);
    }
    if (data->file_special3_id != 0)
    {
        *data->p_file_special3 = lbRelocGetStatusBufferFile(data->file_special3_id);
    }
    if (data->file_special4_id != 0)
    {
        *data->p_file_special4 = lbRelocGetStatusBufferFile(data->file_special4_id);
    }
}

void ftManagerSetupFilesAllKind(s32 fkind)
{
    ftManagerSetupFilesMainKind(fkind);
    ftManagerSetupFilesKind(fkind);
}

void syMallocReset(SYMallocRegion* bp)
{
    bp->ptr = bp->start;
}