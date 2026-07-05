#ifndef _IN_EXT_TEX_H
#define _IN_EXT_TEX_H

#include <PR/ultratypes.h>

#define MASK_FONT_OUTLINE 0x80

s32 extTexInit();
void extTexFree();
s32 extTexPreloadAll(void);
u8 *extTexLoad(u8 type, u16 id, s32 texnum, u32 *width, u32 *height);
u8 extTexExists(u8 type, u16 id, s32 texnum);
u8 extTexFontID(struct font *font);
void extTexRegisterGeneralPtr(const void *addr, u32 size, u32 texnum);
u8 extTexLookupGeneralPtr(const void *addr, u32 *texnum);
void extTexClearGeneralPtrRegistry(void);

#endif
