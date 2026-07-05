#include <ultra64.h>
#include "constants.h"
#include "bss.h"
#include "data.h"
#include "types.h"

void skyReset(u32 stagenum)
{
	g_SkyStageNum = stagenum;
	g_SkyLightningActive = false;
	g_SkyWindSpeed = 1;

	switch (stagenum) {
	case STAGE_MP_COMPLEX:
	case STAGE_MP_G5BUILDING:
		g_SkyWindSpeed = 0.35f;
		break;
	}
}
