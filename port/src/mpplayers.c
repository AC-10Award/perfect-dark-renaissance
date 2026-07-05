#include "types.h"
#include "constants.h"
#include "game/savebuffer.h"
#include "game/filelist.h"
#include "game/pak.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include <string.h>
#include "fs.h"
#include "mpplayers.h"

#define MPPLAYERFS_VERSION 1
#define MPPLAYERFS_FILENAME "$S/mpplayers.bin"
#define MPPLAYERFS_DEVICESERIAL 1

struct mpplayerfsblock {
	u8 bytes[sizeof(((struct savebuffer *)0)->bytes)];
};

struct mpplayerfsfile {
	u8 version;
	u8 numprofiles;
	struct mpplayerfsblock profiles[MPPLAYERFS_MAXPROFILES];
};

static struct mpplayerfsfile g_MpPlayersFile;

static void mpplayersResetFile(struct mpplayerfsfile *file)
{
	memset(file, 0, sizeof(*file));
	file->version = MPPLAYERFS_VERSION;
}

static bool mpplayersBlockIsUsed(struct mpplayerfsblock *block)
{
	return block->bytes[0] != '\0';
}

static void mpplayersSanitizeFile(struct mpplayerfsfile *file)
{
	s32 writeindex = 0;
	s32 i;

	if (file->version != MPPLAYERFS_VERSION) {
		mpplayersResetFile(file);
		return;
	}

	for (i = 0; i < MPPLAYERFS_MAXPROFILES; i++) {
		if (mpplayersBlockIsUsed(&file->profiles[i])) {
			if (writeindex != i) {
				memcpy(file->profiles[writeindex].bytes, file->profiles[i].bytes,
						sizeof(file->profiles[writeindex].bytes));
			}

			writeindex++;
		}
	}

	for (i = writeindex; i < MPPLAYERFS_MAXPROFILES; i++) {
		memset(file->profiles[i].bytes, 0, sizeof(file->profiles[i].bytes));
	}

	file->numprofiles = writeindex;
}

static s32 mpplayersLoadFile(struct mpplayerfsfile *file)
{
	FILE *f = fsFileOpenRead(MPPLAYERFS_FILENAME);

	if (f == NULL) {
		mpplayersResetFile(file);
		return mpplayersSaveCurrentFile();
	}

	mpplayersResetFile(file);
	fread(file, sizeof(*file), 1, f);
	fsFileFree(f);

	mpplayersSanitizeFile(file);
	return 0;
}

static s32 mpplayersSaveFile(struct mpplayerfsfile *file)
{
	FILE *f;

	file->version = MPPLAYERFS_VERSION;
	mpplayersSanitizeFile(file);

	f = fsFileOpenWrite(MPPLAYERFS_FILENAME);

	if (f == NULL) {
		return -1;
	}

	fwrite(file, sizeof(*file), 1, f);
	fsFileFree(f);

	return 0;
}

s32 mpplayersLoadCurrentFile(void)
{
	return mpplayersLoadFile(&g_MpPlayersFile);
}

s32 mpplayersSaveCurrentFile(void)
{
	return mpplayersSaveFile(&g_MpPlayersFile);
}

void mpplayersCopyAllFromPak(void)
{
	if (fsFileSize(MPPLAYERFS_FILENAME) > 0) {
		return;
	}

#ifndef PLATFORM_N64
	mpplayersResetFile(&g_MpPlayersFile);
	mpplayersSaveCurrentFile();
	return;
#else
	filelistCreate(1, FILETYPE_MPPLAYER);
	filelistsTick();

	mpplayersResetFile(&g_MpPlayersFile);

	for (s32 i = 0; i < g_FileLists[1]->numfiles && i < MPPLAYERFS_MAXPROFILES; i++) {
		struct savebuffer buffer;
		struct filelistfile *file = &g_FileLists[1]->files[i];
		s32 device = pakFindBySerial(file->deviceserial);
		s32 err;

		savebufferClear(&buffer);
		err = pakReadBodyAtGuid(device, file->fileid, buffer.bytes, 0);

		if (err == 0) {
			memcpy(g_MpPlayersFile.profiles[g_MpPlayersFile.numprofiles].bytes,
					buffer.bytes,
					sizeof(buffer.bytes));
			g_MpPlayersFile.numprofiles++;
		}
	}

	mpplayersSaveCurrentFile();
#endif
}

s32 mpplayersGetCount(void)
{
	return g_MpPlayersFile.numprofiles;
}

bool mpplayersHasRoomForNewProfile(void)
{
	return g_MpPlayersFile.numprofiles < MPPLAYERFS_MAXPROFILES;
}

void mpplayersGetOverview(s32 index, char *name, u32 *playtime)
{
	if (index < 0 || index >= g_MpPlayersFile.numprofiles) {
		strcpy(name, "<empty>\n");
		*playtime = 0;
		return;
	}

	mpplayerfileGetOverview((char *)g_MpPlayersFile.profiles[index].bytes, name, playtime);

	if (name[0] == '\0') {
		strcpy(name, "<unnamed>\n");
	}
}

s32 mpplayersGetSlotFromFileGuid(struct fileguid *guid)
{
	if (guid->deviceserial != MPPLAYERFS_DEVICESERIAL || guid->fileid <= 0) {
		return -1;
	}

	if (guid->fileid > g_MpPlayersFile.numprofiles) {
		return -1;
	}

	return guid->fileid - 1;
}

s32 mpplayersSavePlayer(s32 playernum, s32 slotindex)
{
	struct savebuffer buffer;

	if (slotindex < 0) {
		if (!mpplayersHasRoomForNewProfile()) {
			return -1;
		}

		slotindex = g_MpPlayersFile.numprofiles++;
	}

	if (slotindex >= MPPLAYERFS_MAXPROFILES) {
		return -1;
	}

	savebufferClear(&buffer);
	mpplayerfileSaveWad(playernum, &buffer);

	memset(g_MpPlayersFile.profiles[slotindex].bytes, 0, sizeof(g_MpPlayersFile.profiles[slotindex].bytes));
	memcpy(g_MpPlayersFile.profiles[slotindex].bytes, buffer.bytes, sizeof(buffer.bytes));

	if (slotindex >= g_MpPlayersFile.numprofiles) {
		g_MpPlayersFile.numprofiles = slotindex + 1;
	}

	if (mpplayersSaveCurrentFile() != 0) {
		return -1;
	}

	return slotindex;
}

s32 mpplayersLoadPlayer(s32 playernum, s32 slotindex)
{
	struct savebuffer buffer;

	if (slotindex < 0 || slotindex >= g_MpPlayersFile.numprofiles) {
		return -1;
	}

	savebufferClear(&buffer);
	memcpy(buffer.bytes, g_MpPlayersFile.profiles[slotindex].bytes, sizeof(buffer.bytes));
	mpplayerfileLoadWad(playernum, &buffer, 1);
	g_PlayerConfigsArray[playernum].handicap = 0x80;
	g_PlayerConfigsArray[playernum].fileguid.fileid = slotindex + 1;
	g_PlayerConfigsArray[playernum].fileguid.deviceserial = MPPLAYERFS_DEVICESERIAL;

	return 0;
}
