#ifndef IN_GAME_GAMEFILE_H
#define IN_GAME_GAMEFILE_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

u32 gamefileHasFlag(u32 value);
void gamefileSetFlag(u32 value);
void gamefileUnsetFlag(u32 value);
void gamefilePrintFlags(void);
void gamefileApplyOptions(struct gamefile *file);
void gamefileLoadDefaults(struct gamefile *file);
s32 gamefileLoad(s32 device);
s32 gamefileSave(s32 device, s32 filenum, u16 deviceserial);
u8 gamefileGetLmsCompletion(s32 stageindex);
bool gamefileIsLmsDifficultyComplete(s32 stageindex, s32 difficultyindex);
void gamefileSetLmsDifficultyComplete(s32 stageindex, s32 difficultyindex, bool completed);
void gamefileGetOverview(char *arg0, char *name, u8 *stage, u8 *difficulty, u32 *time);
void gamefileUnlockEverything(void);
void gamefileUnlockMpFeatures(void);

#endif
