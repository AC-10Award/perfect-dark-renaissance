#include <string.h>
#include <ultra64.h>
#include "constants.h"
#include "bss.h"
#include "lib/dma.h"
#include "lib/memp.h"
#include "data.h"
#include "types.h"

void texInit(void)
{
	extern u8 EXT_SEG _textureslistSegmentRomStart;
	extern u8 EXT_SEG _textureslistSegmentRomEnd;

	u32 len = ((REF_SEG _textureslistSegmentRomEnd - REF_SEG _textureslistSegmentRomStart) + 15) & -16;

#ifdef PLATFORM_N64
	g_Textures = mempAlloc(len, MEMPOOL_PERMANENT);
#else
	u32 alloclen = (NUM_TEXTURES + 1) * sizeof(*g_Textures);
	g_Textures = mempAlloc(alloclen, MEMPOOL_PERMANENT);
	g_NumStockTextures = len / sizeof(*g_Textures) - 1;

	memset(g_Textures, 0, alloclen);
#endif

	dmaExec(g_Textures, (romptr_t) REF_SEG _textureslistSegmentRomStart, len);
}
