#ifndef _IN_MPPLAYERS_H
#define _IN_MPPLAYERS_H

#include "types.h"

#define MPPLAYERFS_MAXPROFILES 64

s32 mpplayersLoadCurrentFile(void);
s32 mpplayersSaveCurrentFile(void);
void mpplayersCopyAllFromPak(void);

s32 mpplayersGetCount(void);
bool mpplayersHasRoomForNewProfile(void);
void mpplayersGetOverview(s32 index, char *name, u32 *playtime);
s32 mpplayersGetSlotFromFileGuid(struct fileguid *guid);
s32 mpplayersSavePlayer(s32 playernum, s32 slotindex);
s32 mpplayersLoadPlayer(s32 playernum, s32 slotindex);

#endif
