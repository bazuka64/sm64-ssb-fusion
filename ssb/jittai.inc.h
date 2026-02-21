LBFileNode sSCVSBattleStatusBuffer[100];
LBFileNode sSCVSBattleForceStatusBuffer[7];
LBInternBuffer sLBRelocInternBuffer;
LBTableEntry sLBRelocTableEntries[nLBFileLocationEnumCount];
LBTableEntry *sLBRelocCurrentTableEntry;
LBTableEntry *sLBRelocNextTableEntry;

SYMallocRegion gSYTaskmanGeneralHeap;

void *gFTNessFileMain;
void *gFTNessFileMainMotion;
void *gFTNessFileModel;
void *gFTNessFileSpecial1;
void *gFTNessFileSpecial2;
void *gFTNessFileSpecial3;
s32 gFTNessParticleBankID;

FTData dFTNessData =
{
    0xef,
    0xee,
    0x00000000,
    0x14f,
    0x151,
    0xf0,
    0x160,
    0x150,
    0x00000000,
    0x00000000,
    &gFTNessFileMain,
    &gFTNessFileMainMotion,
    0x00000000,
    &gFTNessFileModel,
    0x00000000,
    &gFTNessFileSpecial1,
    &gFTNessFileSpecial2,
    &gFTNessFileSpecial3,
    0x00000000,
    &gFTNessParticleBankID,
    0,//&particles_unk1_scb_ROM_START,
    0,//&particles_unk1_scb_ROM_END,
    0,//&particles_unk1_txb_ROM_START,
    0,//&particles_unk1_txb_ROM_END,
    0x000005BC, // o_attributes
    0,//&dFTNessMotionDescs,
    0,//&dFTNessSubMotionDescs,
    0x000000DC,
    0,//&dFTNessSubMotionDescsCount,
    0x00000000
};

u32 *sLBRelocExternFileIDs;
s32 sLBRelocExternFileIDsNum;
s32 sLBRelocExternFileIDsMax;
void *sLBRelocExternFileHeap;

void *sSYDmaVpkBuffer;
size_t sSYDmaVpkBufferSize;
uintptr_t sSYDmaVpkDevAddr;