#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include "gbiex.h"
#include "types.h"

#include "constants.h"
#include "system.h"
#include "fs.h"
#include "data.h"
#include "romdata.h"
#include "ext_tex.h"

#define EXT_TEX_DIRNAME "ext_tex"
#define FONT_OUTLINES_DIR "outlines"

static char extTexPath[FS_MAXPATH + 1];

#define MAX_EXT_TEX 8192
#define NUM_FONTS 5
const u16 IDMASK_FONT_OUTLINE = MASK_FONT_OUTLINE << 8;


struct ExtTexture
{
	u8 *texdata;
	s32 texnum;
	u32 width;
	u32 height;
	char extension[5];
	char *filepath;
};

struct ModelTextures
{
	s16 fileNum;
	s16 numTextures;
	struct ExtTexture *textures;
};

static struct ExtTexture extTextures[MAX_EXT_TEX];
static struct ExtTexture extTexturesByMod[5][MAX_EXT_TEX];

static struct ModelTextures *modelTextures;
static s32 numModels;
static s32 g_ExtTexGeneralCount;
static s32 g_ExtTexFontCount;
static s32 g_ExtTexModelCount;
static s32 g_ExtTexGeneralCountByMod[5];

#define MAX_GENERAL_PTRS 8192

struct GeneralTexturePtrEntry
{
	const void *addr;
	u32 size;
	u32 texnum;
};

static struct GeneralTexturePtrEntry g_GeneralTexturePtrs[MAX_GENERAL_PTRS];
static s32 g_NumGeneralTexturePtrs;

static u8 *extTexLoadInternal(struct ExtTexture *tex, const char *path, s32 shouldFlip, u32 *width, u32 *height);
static s32 extTexPreloadOne(struct ExtTexture *tex, const char *path, u8 type, u16 id, s32 texnum, s32 shouldFlip);

static bool extTexNumIsValid(s32 texnum)
{
	return texnum >= 0 && texnum < MAX_EXT_TEX;
}

#if VERSION == VERSION_PAL_FINAL
#define NCHARS 135
#else
#define NCHARS 94
#endif

static struct ExtTexture fontExtTextures[NUM_FONTS][NCHARS];
static struct ExtTexture fontOutlineExtTextures[NUM_FONTS][NCHARS];

static char *extTexDupString(const char *src)
{
	size_t len;
	char *dst;

	if (src == NULL) {
		return NULL;
	}

	len = strlen(src);
	dst = sysMemAlloc(len + 1);

	if (dst == NULL) {
		return NULL;
	}

	memcpy(dst, src, len + 1);

	return dst;
}

static const char *extTexGetModNamespace(s32 modnum)
{
	switch (modnum) {
	case MOD_GEX: return "gex";
	case MOD_KAKARIKO: return "kakariko";
	case MOD_DARKNOON: return "darknoon";
	case MOD_GOLDFINGER_64: return "goldfinger64";
	default: return NULL;
	}
}

static struct ExtTexture *extTexGetActiveGeneralTexture(s32 texnum)
{
	if (texnum < 0 || texnum >= MAX_EXT_TEX) {
		return NULL;
	}

	if (g_ModNum > MOD_NORMAL && g_ModNum <= MOD_GOLDFINGER_64) {
		struct ExtTexture *modtex = &extTexturesByMod[g_ModNum][texnum];

		if (modtex->texnum >= 0) {
			return modtex;
		}
	}

	return &extTextures[texnum];
}

#define FONT_HANDELGOTHICSM 0
#define FONT_HANDELGOTHICMD 1
#define FONT_HANDELGOTHICXS 2
#define FONT_HANDELGOTHICLG 3
#define FONT_NUMERIC 4

s32 fileInfo(const char *filename, s32 *texNum, char extension[5])
{
	char *ext = strrchr(filename, '.');
	size_t basenameLen;
	size_t prefixLen = 0;
	size_t i;
	char texbuf[16] = { 0 };

	// no extension
	if (!ext) return 1;
	if (filename[0] == '.') return 1;

	basenameLen = (size_t)(ext - filename);

	// empty basename
	if (basenameLen == 0) return 1;

	++ext;
	if (*ext == '\0') return 1;
	strncpy(extension, ext, 5);
	extension[4] = '\0';

	for (i = 0; i < basenameLen; i++) {
		if (!isxdigit((unsigned char)filename[i])) {
			break;
		}
		prefixLen++;
	}

	if (prefixLen == 0 || prefixLen >= sizeof(texbuf)) {
		return 1;
	}

	memcpy(texbuf, filename, prefixLen);
	*texNum = strtol(texbuf, NULL, 16);

	return 0;
}

static s32 fontFileInfo(const char *filename, s32 *texNum, char extension[5])
{
	char *ext = strrchr(filename, '.');
	size_t basenameLen;
	char texbuf[3] = { 0 };

	if (fileInfo(filename, texNum, extension) == 0) {
		return 0;
	}

	// no extension
	if (!ext) return 1;
	if (filename[0] == '.') return 1;

	basenameLen = (size_t)(ext - filename);

	// Require "-xx.ext" so prefixed font names don't accidentally match other numbers.
	if (basenameLen < 3 || filename[basenameLen - 3] != '-') {
		return 1;
	}

	if (!isxdigit((unsigned char)filename[basenameLen - 2])
			|| !isxdigit((unsigned char)filename[basenameLen - 1])) {
		return 1;
	}

	++ext;
	if (*ext == '\0') return 1;
	strncpy(extension, ext, 5);
	extension[4] = '\0';

	texbuf[0] = filename[basenameLen - 2];
	texbuf[1] = filename[basenameLen - 1];
	*texNum = strtol(texbuf, NULL, 16);

	return 0;
}

static bool extTexIsReservedModNamespace(const char *name)
{
	s32 mod;

	for (mod = MOD_GEX; mod <= MOD_GOLDFINGER_64; mod++) {
		const char *modns = extTexGetModNamespace(mod);

		if (modns && strcmp(name, modns) == 0) {
			return true;
		}
	}

	return false;
}

static bool extTexIsModelDirName(const char *name)
{
	char s = name[0];

	if (s != 'P' && s != 'C' && s != 'G') {
		return false;
	}

	return romdataFileGetNumForName(name) >= 0;
}

static bool extTexShouldSkipRootGeneralDir(const char *name)
{
	return name[0] == 'f'
		|| extTexIsReservedModNamespace(name)
		|| extTexIsModelDirName(name);
}

static void extTexLogDuplicate(const char *kind, const char *scope, s32 texNum, const char *oldpath, const char *newpath)
{
	if (oldpath == NULL || newpath == NULL || strcmp(oldpath, newpath) == 0) {
		return;
	}

	sysLogPrintf(LOG_WARNING,
			"ext_tex: duplicate %s tex=%04x scope=%s replacing %s with %s",
			kind, texNum, scope ? scope : "root", oldpath, newpath);
}

struct ExtTexture *lookupModelTex(u16 fileNum, s32 texNum)
{
	if (fileNum > NUM_FILES) {
		sysLogPrintf(LOG_WARNING, "Invalid fileNum in lookupModelTex: %04x, texNum: %04x", fileNum, texNum);
		return 0;
	}

	struct ModelTextures *modelTex = NULL;
	for (int i = 0; i < numModels; ++i) {
		if (modelTextures[i].fileNum == fileNum) {
			modelTex = &modelTextures[i];
			break;
		}
	}

	if (modelTex == NULL)
		return NULL;

	for (int i = 0; i < modelTex->numTextures; ++i) {
		if (modelTex->textures[i].texnum == texNum)
			return &modelTex->textures[i];
	}

	return NULL;
}

struct ExtTexture *getExtTexture(u8 type, u16 id, s32 texnum)
{
	struct ExtTexture *texlist;
	switch (type) {
		case G_TEXTYPE_NONE:
			return NULL;
		case G_TEXTYPE_GENERAL:
			return extTexGetActiveGeneralTexture(texnum);
		case G_TEXTYPE_MODEL:
			return lookupModelTex(id, texnum);
		case G_TEXTYPE_FONT: {
			if (id & IDMASK_FONT_OUTLINE)
				return &fontOutlineExtTextures[id & ~IDMASK_FONT_OUTLINE][texnum];

			return &fontExtTextures[id][texnum];
		}
		default:
			sysLogPrintf(LOG_WARNING, "Invalid Texture type: %d, texnum: %04x", type, texnum);
			return NULL;
	}
}

u8 extTexExists(u8 type, u16 id, s32 texnum)
{
	struct ExtTexture *tex = getExtTexture(type, id, texnum);
	return tex && tex->texnum >= 0;
}

void extTexClearGeneralPtrRegistry(void)
{
	g_NumGeneralTexturePtrs = 0;
}

void extTexRegisterGeneralPtr(const void *addr, u32 size, u32 texnum)
{
	s32 i;

	if (addr == NULL || size == 0) {
		return;
	}

	for (i = g_NumGeneralTexturePtrs - 1; i >= 0; i--) {
		if (g_GeneralTexturePtrs[i].addr == addr) {
			g_GeneralTexturePtrs[i].size = size;
			g_GeneralTexturePtrs[i].texnum = texnum;
			return;
		}
	}

	if (g_NumGeneralTexturePtrs >= MAX_GENERAL_PTRS) {
		return;
	}

	g_GeneralTexturePtrs[g_NumGeneralTexturePtrs].addr = addr;
	g_GeneralTexturePtrs[g_NumGeneralTexturePtrs].size = size;
	g_GeneralTexturePtrs[g_NumGeneralTexturePtrs].texnum = texnum;
	g_NumGeneralTexturePtrs++;
}

u8 extTexLookupGeneralPtr(const void *addr, u32 *texnum)
{
	s32 i;

	if (addr == NULL || texnum == NULL) {
		return 0;
	}

	for (i = g_NumGeneralTexturePtrs - 1; i >= 0; i--) {
		const uintptr_t start = (uintptr_t)g_GeneralTexturePtrs[i].addr;
		const uintptr_t end = start + g_GeneralTexturePtrs[i].size;
		const uintptr_t candidate = (uintptr_t)addr;

		if (candidate >= start && candidate < end) {
			*texnum = g_GeneralTexturePtrs[i].texnum;
			return 1;
		}
	}

	return 0;
}

char *resolveFontname(const u8 fontId)
{
	switch (fontId) {
		case FONT_HANDELGOTHICSM: return "fonthandelgothicsm";
		case FONT_HANDELGOTHICMD: return "fonthandelgothicmd";
		case FONT_HANDELGOTHICXS: return "fonthandelgothicxs";
		case FONT_HANDELGOTHICLG: return "fonthandelgothiclg";
		case FONT_NUMERIC: return "fontnumeric";
		default: return "";
	}
}

u8 getTexPath(char *dst, u8 type, u16 id, s32 texnum)
{
	struct ExtTexture *tex;
	const char *name;

	switch (type) {
		case G_TEXTYPE_GENERAL: {
			tex = extTexGetActiveGeneralTexture(texnum);

			if (tex == NULL || tex->texnum < 0) {
				return 1;
			}

			const char *modns = extTexGetModNamespace(g_ModNum);

			if (modns && tex == &extTexturesByMod[g_ModNum][texnum]) {
				if (tex->filepath != NULL) {
					snprintf(dst, FS_MAXPATH, "%s/%s/%s", extTexPath, modns, tex->filepath);
				} else {
					snprintf(dst, FS_MAXPATH, "%s/%s/%04x.%s", extTexPath, modns, texnum, tex->extension);
				}
			} else {
				if (tex->filepath != NULL) {
					snprintf(dst, FS_MAXPATH, "%s/%s", extTexPath, tex->filepath);
				} else {
					snprintf(dst, FS_MAXPATH, "%s/%04x.%s", extTexPath, texnum, tex->extension);
				}
			}
			return 0;
		}
		case G_TEXTYPE_FONT: {
			name = resolveFontname(id & ~IDMASK_FONT_OUTLINE);

			if (id & IDMASK_FONT_OUTLINE) {
				tex = &fontOutlineExtTextures[id & ~IDMASK_FONT_OUTLINE][texnum];
				if (tex->filepath != NULL) {
					snprintf(dst, FS_MAXPATH, "%s/%s", extTexPath, tex->filepath);
				} else {
					snprintf(dst, FS_MAXPATH, "%s/%s/" FONT_OUTLINES_DIR "/%02x.%s", extTexPath, name, texnum, tex->extension);
				}
				return 0;
			}

			tex = &fontExtTextures[id][texnum];
			if (tex->filepath != NULL) {
				snprintf(dst, FS_MAXPATH, "%s/%s", extTexPath, tex->filepath);
			} else {
				snprintf(dst, FS_MAXPATH, "%s/%s/%02x.%s", extTexPath, name, texnum, tex->extension);
			}
			return 0;
		}
		case G_TEXTYPE_MODEL: {
			name = romdataFileGetName(id);
			tex = lookupModelTex(id, texnum);
			snprintf(dst, FS_MAXPATH, "%s/%s/%05x.%s", extTexPath, name, texnum, tex->extension);
			return 0;
		}
		default: return 1;
	}
}

u8 *extTexLoad(u8 type, u16 id, s32 texnum, u32 *width, u32 *height)
{
	char path[FS_MAXPATH];
	u8 err = getTexPath(path, type, id, texnum);
	s32 shouldFlip;

	if (err) {
		sysLogPrintf(LOG_WARNING, "Invalid type in extTexLoad: %d, id: 04x, texnum: %04x", type, id, texnum);
		return 0;
	}

	struct ExtTexture *tex = getExtTexture(type, id, texnum);

	if (!tex) {
		sysLogPrintf(LOG_WARNING, "Unable to load texture: %05x", texnum);
		return NULL;
	}

	// General textures are stored upside down relative to the game's sampling path,
	// but font glyph bitmaps are already authored in the orientation the text renderer expects.
	shouldFlip = type != G_TEXTYPE_FONT;
	tex->texdata = extTexLoadInternal(tex, path, shouldFlip, width, height);

	if (tex->texdata == NULL) {
		sysLogPrintf(LOG_WARNING, "ext_tex: failed to load type=%d id=%04x tex=%04x path=%s (%s)",
				type, id, texnum, path, stbi_failure_reason());
	} else {
		sysLogPrintf(LOG_NOTE, "ext_tex: loaded type=%d id=%04x tex=%04x from %s (%ux%u)",
				type, id, texnum, path, tex->width, tex->height);
	}

	return tex->texdata;
}

u8 extTexFontID(struct font *font) {
	if (font == g_FontHandelGothicSm)
		return FONT_HANDELGOTHICSM;
	else if (font == g_FontHandelGothicMd)
		return FONT_HANDELGOTHICMD;
	else if (font == g_FontHandelGothicXs)
		return FONT_HANDELGOTHICXS;
	else if (font == g_FontHandelGothicLg)
		return FONT_HANDELGOTHICLG;
	else if (font == g_FontNumeric)
		return FONT_NUMERIC;

	return 0xff;
}

u8 resolveFontID(const char *fontname)
{
	if (strcmp(fontname, "fonthandelgothicsm") == 0)
		return FONT_HANDELGOTHICSM;
	else if (strcmp(fontname, "fonthandelgothicmd") == 0)
		return FONT_HANDELGOTHICMD;
	else if (strcmp(fontname, "fonthandelgothicxs") == 0)
		return FONT_HANDELGOTHICXS;
	else if (strcmp(fontname, "fonthandelgothiclg") == 0)
		return FONT_HANDELGOTHICLG;
	else if (strcmp(fontname, "fontnumeric") == 0)
		return FONT_NUMERIC;

	return 0xff;
}

void setTexPath(struct ExtTexture *texlist, s32 index, s32 texNum, char extension[5], const char *filepath)
{
	struct ExtTexture *tex = &texlist[index];

	if (tex->filepath != NULL) {
		sysMemFree(tex->filepath);
		tex->filepath = NULL;
	}

	tex->texnum = texNum;
	tex->width = 0;
	tex->height = 0;
	strcpy(tex->extension, extension);
	tex->filepath = extTexDupString(filepath);
}

void setTex(struct ExtTexture *texlist, s32 index, s32 texNum, char extension[5])
{
	setTexPath(texlist, index, texNum, extension, NULL);
}

static u8 *extTexLoadInternal(struct ExtTexture *tex, const char *path, s32 shouldFlip, u32 *width, u32 *height)
{
	u32 channels;

	if (tex == NULL || path == NULL) {
		return NULL;
	}

	if (tex->texdata != NULL) {
		if (width) {
			*width = tex->width;
		}

		if (height) {
			*height = tex->height;
		}

		return tex->texdata;
	}

	stbi_set_flip_vertically_on_load(shouldFlip);
	tex->texdata = stbi_load(path, (s32 *)&tex->width, (s32 *)&tex->height, (s32 *)&channels, 4);
	stbi_set_flip_vertically_on_load(1);

	if (tex->texdata == NULL) {
		return NULL;
	}

	if (width) {
		*width = tex->width;
	}

	if (height) {
		*height = tex->height;
	}

	return tex->texdata;
}

static s32 extTexPreloadOne(struct ExtTexture *tex, const char *path, u8 type, u16 id, s32 texnum, s32 shouldFlip)
{
	u32 width;
	u32 height;

	if (tex == NULL || tex->texnum < 0) {
		return 0;
	}

	if (extTexLoadInternal(tex, path, shouldFlip, &width, &height) == NULL) {
		sysLogPrintf(LOG_WARNING, "ext_tex: preload failed type=%d id=%04x tex=%04x path=%s (%s)",
				type, id, texnum, path, stbi_failure_reason());
		return 0;
	}

	sysLogPrintf(LOG_NOTE, "ext_tex: preloaded type=%d id=%04x tex=%04x from %s (%ux%u)",
			type, id, texnum, path, width, height);
	return 1;
}

static void readGeneralTexturesRecursive(const char *rootpath, const char *subpath, struct ExtTexture *texlist, s32 *count, bool skipreserved)
{
	char scanpath[FS_MAXPATH];
	DIR *dr;
	struct dirent *de;

	if (subpath != NULL && subpath[0] != '\0') {
		snprintf(scanpath, sizeof(scanpath), "%s/%s", rootpath, subpath);
	} else {
		snprintf(scanpath, sizeof(scanpath), "%s", rootpath);
	}

	dr = opendir(scanpath);

	if (dr == NULL) {
		return;
	}

	while ((de = readdir(dr)) != NULL) {
		const char *name = de->d_name;
		char filepath[FS_MAXPATH];
		char relpath[FS_MAXPATH];
		struct stat stbuf;
		s32 texNum = 0;
		char extension[5] = { 0 };
		s32 err;

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}

		if (subpath != NULL && subpath[0] != '\0') {
			snprintf(relpath, sizeof(relpath), "%s/%s", subpath, name);
		} else {
			snprintf(relpath, sizeof(relpath), "%s", name);
		}

		snprintf(filepath, sizeof(filepath), "%s/%s", rootpath, relpath);

		if (stat(filepath, &stbuf) == -1) {
			continue;
		}

		if (S_ISDIR(stbuf.st_mode)) {
			if (skipreserved && extTexShouldSkipRootGeneralDir(name)) {
				continue;
			}

			readGeneralTexturesRecursive(rootpath, relpath, texlist, count, false);
			continue;
		}

		err = fileInfo(name, &texNum, extension);

		if (err || !extTexNumIsValid(texNum)) {
			continue;
		}

		extTexLogDuplicate("general", rootpath, texNum, texlist[texNum].filepath, relpath);
		setTexPath(texlist, texNum, texNum, extension, relpath);

		if (count) {
			(*count)++;
		}
	}

	closedir(dr);
}

void readModelTextures(const char *path, s16 fileNum, s32 *modelOffset, struct ModelTextures *modelTex)
{
	DIR *dr = opendir(path);
	struct dirent *de;

	s32 MAX_TEX = 16;
	modelTex->textures = sysMemAlloc(MAX_TEX * sizeof(struct ExtTexture));
	modelTex->numTextures = 0;
	modelTex->fileNum = fileNum;

	if (dr == NULL || modelTex->textures == NULL) {
		if (dr != NULL) {
			closedir(dr);
		}
		return;
	}

	char extension[5] = { 0 };

	while ((de = readdir(dr)) != NULL) {
		const char *name = de->d_name;
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

		s32 texNum;
		s32 err = fileInfo(name, &texNum, extension);
		// no extension: skip
		if (err) continue;

		// allocate more memory for model textures if needed
		if (modelTex->numTextures >= MAX_TEX) {
			struct ExtTexture *newTextures;

			MAX_TEX *= 2;
			newTextures = sysMemRealloc(modelTex->textures, MAX_TEX * sizeof(struct ExtTexture));
			if (!newTextures) {
				break;
			}
			modelTex->textures = newTextures;
		}

		setTex(modelTex->textures, modelTex->numTextures, texNum, extension);
		modelTex->numTextures++;
	}
	closedir(dr);

	// shrink the textures array to the actual number of textures found
	s32 numTex = modelTex->numTextures;

	if (numTex > 0)
		modelTex->textures = sysMemRealloc(modelTex->textures, numTex * sizeof(struct ExtTexture));

	for (int i = 0; i < modelTex->numTextures; ++i) {
		modelTex->textures[i].texdata = 0;
	}
}

void readFontTextures(const char *path, const char *fontName, const char *relpath)
{
	DIR *dr = opendir(path);
	struct dirent *de;

	u8 fontID = resolveFontID(fontName);
	char extension[5] = { 0 };

	char outlinesPath[FS_MAXPATH];
	sprintf(outlinesPath , "%s/" FONT_OUTLINES_DIR, path);
	u8 outlines = false;

	if (dr == NULL || fontID == 0xff) {
		if (dr != NULL) {
			closedir(dr);
		}

		return;
	}

	while (true) {
		de = readdir(dr);
		// after done processing the font folder, do the same for the outlines folder if any
		if (de == NULL) {
			if (outlines) break;

			outlines = true;
			closedir(dr);
			dr = opendir(outlinesPath);

			if (dr == NULL) {
				break;
			}

			de = readdir(dr);

			if (de == NULL) break;
		}

		const char *name = de->d_name;
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

		s32 texNum;
		s32 err = fontFileInfo(name, &texNum, extension);
		char glyphPath[FS_MAXPATH];
		// no extension: skip
		if (err) continue;
		if (texNum < 0 || texNum >= NCHARS) continue;

		if (outlines) {
			snprintf(glyphPath, sizeof(glyphPath), "%s/%s/%s", relpath, FONT_OUTLINES_DIR, name);
			extTexLogDuplicate("font-outline", fontName, texNum, fontOutlineExtTextures[fontID][texNum].filepath, glyphPath);
			setTexPath(fontOutlineExtTextures[fontID], texNum, texNum, extension, glyphPath);
		} else {
			snprintf(glyphPath, sizeof(glyphPath), "%s/%s", relpath, name);
			extTexLogDuplicate("font", fontName, texNum, fontExtTextures[fontID][texNum].filepath, glyphPath);
			setTexPath(fontExtTextures[fontID], texNum, texNum, extension, glyphPath);
		}
	}

	if (dr != NULL) {
		closedir(dr);
	}
}

static void readFontDirectoriesRecursive(const char *rootpath, const char *subpath, bool skipreserved)
{
	char scanpath[FS_MAXPATH];
	DIR *dr;
	struct dirent *de;

	if (subpath != NULL && subpath[0] != '\0') {
		snprintf(scanpath, sizeof(scanpath), "%s/%s", rootpath, subpath);
	} else {
		snprintf(scanpath, sizeof(scanpath), "%s", rootpath);
	}

	dr = opendir(scanpath);

	if (dr == NULL) {
		return;
	}

	while ((de = readdir(dr)) != NULL) {
		const char *name = de->d_name;
		char filepath[FS_MAXPATH];
		char relpath[FS_MAXPATH];
		struct stat stbuf;

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}

		if (subpath != NULL && subpath[0] != '\0') {
			snprintf(relpath, sizeof(relpath), "%s/%s", subpath, name);
		} else {
			snprintf(relpath, sizeof(relpath), "%s", name);
		}

		snprintf(filepath, sizeof(filepath), "%s/%s", rootpath, relpath);

		if (stat(filepath, &stbuf) == -1 || !S_ISDIR(stbuf.st_mode)) {
			continue;
		}

		if (resolveFontID(name) != 0xff) {
			readFontTextures(filepath, name, relpath);
			continue;
		}

		if (skipreserved && (extTexIsReservedModNamespace(name) || extTexIsModelDirName(name))) {
			continue;
		}

		readFontDirectoriesRecursive(rootpath, relpath, false);
	}

	closedir(dr);
}

void extTexFree()
{
	extTexClearGeneralPtrRegistry();

	for (int i = 0; i < MAX_EXT_TEX; ++i) {
		if (extTextures[i].texdata)
			stbi_image_free(extTextures[i].texdata);

		extTextures[i].texdata = 0;

		for (int mod = 0; mod <= MOD_GOLDFINGER_64; mod++) {
			if (extTexturesByMod[mod][i].texdata) {
				stbi_image_free(extTexturesByMod[mod][i].texdata);
			}

			extTexturesByMod[mod][i].texdata = 0;
		}
	}

	for (int i = 0; i < NUM_FONTS; ++i) {
		for (int j = 0; j < NCHARS; ++j) {
			if (fontExtTextures[i][j].texdata)
				stbi_image_free(fontExtTextures[i][j].texdata);

			if (fontOutlineExtTextures[i][j].texdata)
				stbi_image_free(fontOutlineExtTextures[i][j].texdata);

			fontExtTextures[i][j].texdata = 0;
			fontOutlineExtTextures[i][j].texdata = 0;
		}
	}

	for (int i = 0; i < numModels; ++i) {
		struct ModelTextures *modelTex = &modelTextures[i];
		for (int j = 0; j < modelTex->numTextures; ++j) {
			if (modelTex->textures[j].texdata)
				stbi_image_free(modelTex->textures[j].texdata);

			modelTex->textures[j].texdata = 0;
		}
	}
}

s32 extTexPreloadAll(void)
{
	s32 count = 0;
	s32 mod;
	s32 i;
	s32 j;

	for (i = 0; i < MAX_EXT_TEX; i++) {
		char path[FS_MAXPATH];

		if (extTextures[i].texnum >= 0) {
			if (extTextures[i].filepath != NULL) {
				snprintf(path, sizeof(path), "%s/%s", extTexPath, extTextures[i].filepath);
			} else {
				snprintf(path, sizeof(path), "%s/%04x.%s", extTexPath, extTextures[i].texnum, extTextures[i].extension);
			}
			count += extTexPreloadOne(&extTextures[i], path, G_TEXTYPE_GENERAL, 0, extTextures[i].texnum, true);
		}

		for (mod = MOD_GEX; mod <= MOD_GOLDFINGER_64; mod++) {
			const char *modns = extTexGetModNamespace(mod);
			struct ExtTexture *tex = &extTexturesByMod[mod][i];

			if (modns && tex->texnum >= 0) {
				if (tex->filepath != NULL) {
					snprintf(path, sizeof(path), "%s/%s/%s", extTexPath, modns, tex->filepath);
				} else {
					snprintf(path, sizeof(path), "%s/%s/%04x.%s", extTexPath, modns, tex->texnum, tex->extension);
				}
				count += extTexPreloadOne(tex, path, G_TEXTYPE_GENERAL, 0, tex->texnum, true);
			}
		}
	}

	for (i = 0; i < NUM_FONTS; i++) {
		const char *fontname = resolveFontname(i);

		for (j = 0; j < NCHARS; j++) {
			char path[FS_MAXPATH];

			if (fontExtTextures[i][j].texnum >= 0) {
				if (fontExtTextures[i][j].filepath != NULL) {
					snprintf(path, sizeof(path), "%s/%s", extTexPath, fontExtTextures[i][j].filepath);
				} else {
					snprintf(path, sizeof(path), "%s/%s/%02x.%s", extTexPath, fontname, fontExtTextures[i][j].texnum, fontExtTextures[i][j].extension);
				}
				count += extTexPreloadOne(&fontExtTextures[i][j], path, G_TEXTYPE_FONT, i, fontExtTextures[i][j].texnum, false);
			}

			if (fontOutlineExtTextures[i][j].texnum >= 0) {
				if (fontOutlineExtTextures[i][j].filepath != NULL) {
					snprintf(path, sizeof(path), "%s/%s", extTexPath, fontOutlineExtTextures[i][j].filepath);
				} else {
					snprintf(path, sizeof(path), "%s/%s/" FONT_OUTLINES_DIR "/%02x.%s", extTexPath, fontname, fontOutlineExtTextures[i][j].texnum, fontOutlineExtTextures[i][j].extension);
				}
				count += extTexPreloadOne(&fontOutlineExtTextures[i][j], path, G_TEXTYPE_FONT, i | IDMASK_FONT_OUTLINE, fontOutlineExtTextures[i][j].texnum, false);
			}
		}
	}

	for (i = 0; i < numModels; i++) {
		struct ModelTextures *modelTex = &modelTextures[i];
		const char *name = romdataFileGetName(modelTex->fileNum);

		for (j = 0; j < modelTex->numTextures; j++) {
			char path[FS_MAXPATH];
			struct ExtTexture *tex = &modelTex->textures[j];

			if (tex->texnum < 0) {
				continue;
			}

			snprintf(path, sizeof(path), "%s/%s/%05x.%s", extTexPath, name, tex->texnum, tex->extension);
			count += extTexPreloadOne(tex, path, G_TEXTYPE_MODEL, modelTex->fileNum, tex->texnum, true);
		}
	}

	sysLogPrintf(LOG_NOTE, "ext_tex: preload complete, cached %d textures", count);

	return count;
}

s32 extTexInit()
{
	const char *path = fsFullPath(EXT_TEX_DIRNAME);
	strcpy(extTexPath, path);
	stbi_set_flip_vertically_on_load(1);
	g_ExtTexGeneralCount = 0;
	g_ExtTexFontCount = 0;
	g_ExtTexModelCount = 0;
	memset(g_ExtTexGeneralCountByMod, 0, sizeof(g_ExtTexGeneralCountByMod));
	extTexClearGeneralPtrRegistry();
	sysLogPrintf(LOG_NOTE, "ext_tex: init path resolved to %s", extTexPath);

	for (int i = 0; i < MAX_EXT_TEX; ++i) {
		extTextures[i].texnum = -1;
		extTextures[i].texdata = 0;
		extTextures[i].width = 0;
		extTextures[i].height = 0;
		extTextures[i].filepath = NULL;

		for (int mod = 0; mod <= MOD_GOLDFINGER_64; mod++) {
			extTexturesByMod[mod][i].texnum = -1;
			extTexturesByMod[mod][i].texdata = 0;
			extTexturesByMod[mod][i].width = 0;
			extTexturesByMod[mod][i].height = 0;
			extTexturesByMod[mod][i].filepath = NULL;
		}
	}

	for (int i = 0; i < NUM_FONTS; ++i) {
		for (int j = 0; j < NCHARS; ++j) {
			fontExtTextures[i][j].texnum = -1;
			fontExtTextures[i][j].texdata = 0;
			fontExtTextures[i][j].width = 0;
			fontExtTextures[i][j].height = 0;
			fontExtTextures[i][j].filepath = NULL;

			fontOutlineExtTextures[i][j].texnum = -1;
			fontOutlineExtTextures[i][j].texdata = 0;
			fontOutlineExtTextures[i][j].width = 0;
			fontOutlineExtTextures[i][j].height = 0;
			fontOutlineExtTextures[i][j].filepath = NULL;
		}
	}

	struct dirent *de;
	DIR *dr = opendir(extTexPath);

	if (dr == NULL) {
		sysLogPrintf(LOG_NOTE, "ext_tex: directory not found at %s", extTexPath);
		return 0;
	}

	char filepath[FS_MAXPATH];
	s32 modelOffset = 0;

	s32 MAX_MODELS = 16;
	numModels = 0;
	modelTextures = sysMemAlloc(MAX_MODELS * sizeof(struct ModelTextures));
	if (!modelTextures) {
		closedir(dr);
		return 0;
	}

	while ((de = readdir(dr)) != NULL) {
		const char *name = de->d_name;
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

		struct stat stbuf;
		sprintf(filepath , "%s/%s", extTexPath, de->d_name);
		if (stat(filepath, &stbuf) == -1) {
			sysLogPrintf(LOG_WARNING, "Unable to stat file: %s\n", filepath);
			continue;
		}

		// is a directory
		if (S_ISDIR(stbuf.st_mode)) {
			// models
			char s = name[0];
			if (s == 'P' || s == 'C' || s == 'G') {
				s16 fileNum = (s16)romdataFileGetNumForName(name);
				if (fileNum < 0) {
					sysLogPrintf(LOG_WARNING, "extTexInit invalid file: %s\n", name);
					continue;
				}

				// allocate more memory if necessary
				if (numModels >= MAX_MODELS) {
					struct ModelTextures *newModelTextures;

					MAX_MODELS *= 2;
					newModelTextures = sysMemRealloc(modelTextures, MAX_MODELS * sizeof(struct ModelTextures));
					if (!newModelTextures) {
						closedir(dr);
						return 0;
					}
					modelTextures = newModelTextures;
				}

				struct ModelTextures *modelTex = &modelTextures[numModels++];
				readModelTextures(filepath, fileNum, &modelOffset, modelTex);
				g_ExtTexModelCount += modelTex->numTextures;
			}
		}
	}

	closedir(dr);

	readFontDirectoriesRecursive(extTexPath, "", true);

	readGeneralTexturesRecursive(extTexPath, "", extTextures, &g_ExtTexGeneralCount, true);

	for (int mod = MOD_GEX; mod <= MOD_GOLDFINGER_64; mod++) {
		const char *modns = extTexGetModNamespace(mod);

		if (modns) {
			char modpath[FS_MAXPATH];
			snprintf(modpath, sizeof(modpath), "%s/%s", extTexPath, modns);
			readGeneralTexturesRecursive(modpath, "", extTexturesByMod[mod], &g_ExtTexGeneralCountByMod[mod], false);
		}
	}

	// shrink this array to the actual number of model folders found
	if (numModels > 0)
		modelTextures = sysMemRealloc(modelTextures, numModels * sizeof(struct ModelTextures));

	for (int i = 0; i < NUM_FONTS; ++i) {
		for (int j = 0; j < NCHARS; ++j) {
			if (fontExtTextures[i][j].texnum >= 0) {
				g_ExtTexFontCount++;
			}
			if (fontOutlineExtTextures[i][j].texnum >= 0) {
				g_ExtTexFontCount++;
			}
		}
	}

	sysLogPrintf(LOG_NOTE, "ext_tex: indexed %d general, %d font, %d model textures",
			g_ExtTexGeneralCount, g_ExtTexFontCount, g_ExtTexModelCount);
	sysLogPrintf(LOG_NOTE, "ext_tex: indexed namespaced general textures gex=%d kakariko=%d darknoon=%d goldfinger64=%d",
			g_ExtTexGeneralCountByMod[MOD_GEX],
			g_ExtTexGeneralCountByMod[MOD_KAKARIKO],
			g_ExtTexGeneralCountByMod[MOD_DARKNOON],
			g_ExtTexGeneralCountByMod[MOD_GOLDFINGER_64]);

	return 0;
}
