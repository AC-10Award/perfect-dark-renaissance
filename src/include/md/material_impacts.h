#ifndef IN_MD_MATERIAL_IMPACTS_H
#define IN_MD_MATERIAL_IMPACTS_H

#include <ultra64.h>
#include "types.h"

void mdMaterialImpactPlayBodyHit(bool enabled);
void mdMaterialImpactPlayTerminationMetalHit(void);
void mdMaterialImpactsTick(void);
bool mdMaterialImpactTryPlayCustomBgMetalHit(s32 stagenum, s32 texturenum);
bool mdMaterialImpactTryPlayCustomBgWallHit(s32 stagenum, s32 texturenum);
void mdMaterialImpactPlayTileHit(void);
void mdMaterialImpactPlayDoorDebris(void);

#endif
