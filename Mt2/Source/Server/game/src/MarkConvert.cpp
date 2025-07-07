#include "stdafx.h"
#include "MarkManager.h"

#ifdef __WIN32__
#include <direct.h>
#endif

#define OLD_MARK_INDEX_FILENAME "guild_mark.idx"
#define OLD_MARK_DATA_FILENAME "guild_mark.tga"

static Pixel * LoadOldGuildMarkImageFile()
{
	FILE * fp = fopen(OLD_MARK_DATA_FILENAME, "rb");

	if (!fp)
	{
		sys_err("cannot open %s", OLD_MARK_INDEX_FILENAME);
		return NULL;
	}

	int dataSize = 512 * 512 * sizeof(Pixel);
	Pixel * dataPtr = (Pixel *) malloc(dataSize);

	fread(dataPtr, dataSize, 1, fp);

	fclose(fp);

	return dataPtr;
}

bool GuildMarkConvert(const std::vector<DWORD> & vecGuildID)
{
	// Æú´õ »ý¼º
#ifndef __WIN32__
	mkdir("mark", S_IRWXU);
#else
	_mkdir("mark");
#endif

	// ÀÎµ¦½º ÆÄÀÏÀÌ ÀÖ³ª? 
#ifndef __WIN32__
	if (0 != access(OLD_MARK_INDEX_FILENAME, F_OK))
#else
	if (0 != _access(OLD_MARK_INDEX_FILENAME, 0))
#endif
		return true;

	// ÀÎµ¦½º ÆÄÀÏ ¿­±â
	FILE* fp = fopen(OLD_MARK_INDEX_FILENAME, "r");

	if (NULL == fp)
		return false;

	// ÀÌ¹ÌÁö ÆÄÀÏ ¿­±â
	Pixel * oldImagePtr = LoadOldGuildMarkImageFile();

	if (NULL == oldImagePtr)
	{
		fclose(fp);
		return false;
	}

	/*
	// guild_mark.tga°¡ ½ÇÁ¦ targa ÆÄÀÏÀÌ ¾Æ´Ï°í, 512 * 512 * 4 Å©±âÀÇ raw ÆÄÀÏÀÌ´Ù.
	// ´«À¸·Î È®ÀÎÇÏ±â À§ÇØ ½ÇÁ¦ targa ÆÄÀÏ·Î ¸¸µç´Ù.
	CGuildMarkImage * pkImage = new CGuildMarkImage;
	pkImage->Build("guild_mark_real.tga");
	pkImage->Load("guild_mark_real.tga");
	pkImage->PutData(0, 0, 512, 512, oldImagePtr);
	pkImage->Save("guild_mark_real.tga");
	*/
	sys_log(0, "Guild Mark Converting Start.");

	char line[256];
	DWORD guild_id;
	DWORD mark_id;
	Pixel mark[SGuildMark::SIZE];

	while (fgets(line, sizeof(line)-1, fp))
	{
		sscanf(line, "%u %u", &guild_id, &mark_id);

		if (find(vecGuildID.begin(), vecGuildID.end(), guild_id) == vecGuildID.end())
		{
			sys_log(0, "  skipping guild ID %u", guild_id);
			continue;
		}

		// mark id -> ÀÌ¹ÌÁö¿¡¼­ÀÇ À§Ä¡ Ã£±â
		uint row = mark_id / 32;
		uint col = mark_id % 32;

		if (row >= 42)
		{
			sys_err("invalid mark_id %u", mark_id);
			continue;
		}

		uint sx = col * 16;
		uint sy = row * 12;

		Pixel * src = oldImagePtr + sy * 512 + sx;
		Pixel * dst = mark;

		// ¿¾³¯ ÀÌ¹ÌÁö¿¡¼­ ¸¶Å© ÇÑ°³ º¹»ç
		for (int y = 0; y != SGuildMark::HEIGHT; ++y)
		{
			for (int x = 0; x != SGuildMark::WIDTH; ++x)
				*(dst++) = *(src+x);

			src += 512;
		}

		// »õ ±æµå ¸¶Å© ½Ã½ºÅÛ¿¡ ³Ö´Â´Ù.
		CGuildMarkManager::instance().SaveMark(guild_id, (BYTE *) mark);
		line[0] = '\0';
	}

	free(oldImagePtr);
	fclose(fp);

	// ÄÁ¹öÆ®´Â ÇÑ¹ø¸¸ ÇÏ¸éµÇ¹Ç·Î ÆÄÀÏÀ» ¿Å°ÜÁØ´Ù.
#ifndef __WIN32__
	system("mv -f guild_mark.idx guild_mark.idx.removable");
	system("mv -f guild_mark.tga guild_mark.tga.removable");
#else
	system("move /Y guild_mark.idx guild_mark.idx.removable");
	system("move /Y guild_mark.tga guild_mark.tga.removable");
#endif

	sys_log(0, "Guild Mark Converting Complete.");

	return true;
}

