﻿#ifndef __INC_METIN_II_MARKIMAGE_H__
#define __INC_METIN_II_MARKIMAGE_H__

#include <IL/il.h>
#include "minilzo.h"
#include <google/protobuf/repeated_field.h>

typedef unsigned int Pixel;

struct SGuildMark
{
	enum
	{
		WIDTH = 16,
		HEIGHT = 12,
		SIZE = WIDTH * HEIGHT,
	};

	///////////////////////////////////////////////////////////////////////////////
	Pixel m_apxBuf[SIZE];	// ½ÇÁ¦ ÀÌ¹ÌÁö

	///////////////////////////////////////////////////////////////////////////////
	void Clear();
	bool IsEmpty();
};

struct SGuildMarkBlock
{
	enum
	{
		MARK_PER_BLOCK_WIDTH = 4,
		MARK_PER_BLOCK_HEIGHT = 4,

		WIDTH = SGuildMark::WIDTH * MARK_PER_BLOCK_WIDTH,
		HEIGHT = SGuildMark::HEIGHT * MARK_PER_BLOCK_HEIGHT,

		SIZE = WIDTH * HEIGHT,
		MAX_COMP_SIZE = (SIZE * sizeof(Pixel)) + ((SIZE * sizeof(Pixel)) >> 4) + 64 + 3 
	};

	///////////////////////////////////////////////////////////////////////////////
	Pixel	m_apxBuf[SIZE];	// ½ÇÁ¦ ÀÌ¹ÌÁö

	BYTE 	m_abCompBuf[MAX_COMP_SIZE];	// ¾ÐÃàµÈ µ¥ÀÌÅÍ
	lzo_uint m_sizeCompBuf;	// ¾ÐÃàµÈ Å©±â
	DWORD	m_crc;			// ¾ÐÃàµÈ µ¥ÀÌÅÍÀÇ CRC

	///////////////////////////////////////////////////////////////////////////////
	DWORD	GetCRC() const;

	void	CopyFrom(const BYTE * pbCompBuf, DWORD dwCompSize, DWORD crc);
	void	Compress(const Pixel * pxBuf);
};

class CGuildMarkImage
{
	public:
		enum
		{
			WIDTH = 512,
			HEIGHT = 512,

			BLOCK_ROW_COUNT = HEIGHT / SGuildMarkBlock::HEIGHT, // 10
			BLOCK_COL_COUNT = WIDTH / SGuildMarkBlock::WIDTH, // 8

			BLOCK_TOTAL_COUNT = BLOCK_ROW_COUNT * BLOCK_COL_COUNT, // 80

			MARK_ROW_COUNT = BLOCK_ROW_COUNT * SGuildMarkBlock::MARK_PER_BLOCK_HEIGHT, // 40
			MARK_COL_COUNT = BLOCK_COL_COUNT * SGuildMarkBlock::MARK_PER_BLOCK_WIDTH, // 32

			MARK_TOTAL_COUNT = MARK_ROW_COUNT * MARK_COL_COUNT, // 1280

			INVALID_MARK_POSITION = 0xffffffff,
		};

		CGuildMarkImage();
		virtual ~CGuildMarkImage();

		void Create();
		void Destroy();

		bool Build(const char * c_szFileName);
		bool Save(const char* c_szFileName);
		bool Load(const char* c_szFileName);

		void PutData(UINT x, UINT y, UINT width, UINT height, void* data);
		void GetData(UINT x, UINT y, UINT width, UINT height, void* data);

		bool SaveMark(DWORD posMark, BYTE * pbMarkImage);
		bool DeleteMark(DWORD posMark);
		bool SaveBlockFromCompressedData(DWORD posBlock, const BYTE * pbComp, DWORD dwCompSize); // ¼­¹ö -> Å¬¶óÀÌ¾ðÆ®

		DWORD GetEmptyPosition(); // ºó ¸¶Å© À§Ä¡¸¦ ¾ò´Â´Ù.

		void GetBlockCRCList(DWORD * crcList);
		void GetDiffBlocks(const ::google::protobuf::RepeatedField<uint32_t>& crc_list, std::map<BYTE, const SGuildMarkBlock*>& mapDiffBlocks);

	private:
		enum
		{
			INVALID_HANDLE = 0xffffffff,
		};

		void	BuildAllBlocks();

		SGuildMarkBlock	m_aakBlock[BLOCK_ROW_COUNT][BLOCK_COL_COUNT];
		Pixel m_apxImage[WIDTH * HEIGHT];

		ILuint m_uImg;
};

#endif
