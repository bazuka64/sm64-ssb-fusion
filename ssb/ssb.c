#include "lbdef.h"
#include <macros.h>
#include "ftdef.h"

#include "struct.h"
#include "jittai.inc.h"

// func
void syDebugPrintf(const char* fmt, ...) {}
void scManagerRunPrintGObjStatus(){}
#include "ssb_memory.inc.h"

void ssb_init() {
    syMallocInit(&gSYTaskmanGeneralHeap, 0x10000, 0x80400000, 0x400000);
    scVSBattleSetupFiles();
    ftManagerSetupFilesAllKind(nFTKindNess);
}