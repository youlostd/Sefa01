#ifndef __INC_METIN_II_GUILDLIB_MARK_MANAGER_H__
#define __INC_METIN_II_GUILDLIB_MARK_MANAGER_H__

#include "MarkImage.h"

class CGuildMarkManager : public singleton<CGuildMarkManager>
{
	public:
		enum
		{
			MAX_IMAGE_COUNT = 5,
			INVALID_MARK_ID = 0xffffffff,
		};

		// Symbol
		struct TGuildSymbol
		{
			DWORD crc;
			std::vector<BYTE> raw;
		};

		CGuildMarkManager();
		virtual ~CGuildMarkManager();

		const TGuildSymbol * GetGuildSymbol(DWORD GID);
		bool LoadSymbol(const char* filename);
		void SaveSymbol(const char* filename);
		void UploadSymbol(DWORD guildID, int iSize, const BYTE* pbyData);

		//
		// Mark
		//
		void SetMarkPathPrefix(const char * prefix);

		bool LoadMarkIndex(); // ¸¶Å© ÀÎµ¦½º ºÒ·¯¿À±â (¼­¹ö¿¡¼­¸¸ »ç¿ë)
		bool SaveMarkIndex(); // ¸¶Å© ÀÎµ¦½º ÀúÀåÇÏ±â

		void LoadMarkImages(); // ¸ðµç ¸¶Å© ÀÌ¹ÌÁö¸¦ ºÒ·¯¿À±â
		void SaveMarkImage(DWORD imgIdx); // ¸¶Å© ÀÌ¹ÌÁö ÀúÀå

		bool GetMarkImageFilename(DWORD imgIdx, std::string & path) const;
		bool AddMarkIDByGuildID(DWORD guildID, DWORD markID);
		DWORD GetMarkImageCount() const;
		const std::map<DWORD, DWORD>& GetMarkMap() const { return m_mapGID_MarkID; }
		DWORD GetMarkCount() const;
		DWORD GetMarkID(DWORD guildID);

		// SERVER
		void CopyMarkIdx(char * pcBuf) const;
		DWORD SaveMark(DWORD guildID, BYTE * pbMarkImage);
		void DeleteMark(DWORD guildID);
		void GetDiffBlocks(DWORD imgIdx, const ::google::protobuf::RepeatedField<uint32_t>& crc_list, std::map<BYTE, const SGuildMarkBlock *> & mapDiffBlocks);

		// CLIENT
		bool SaveBlockFromCompressedData(DWORD imgIdx, DWORD idBlock, const BYTE * pbBlock, DWORD dwSize);
		bool GetBlockCRCList(DWORD imgIdx, DWORD * crcList);

	private:
		// 
		// Mark
		//
		CGuildMarkImage * __NewImage();
		void __DeleteImage(CGuildMarkImage * pkImgDel);

		DWORD __AllocMarkID(DWORD guildID);

		CGuildMarkImage * __GetImage(DWORD imgIdx);
		CGuildMarkImage * __GetImagePtr(DWORD idMark);

		std::map<DWORD, CGuildMarkImage *> m_mapIdx_Image; // index = image index
		std::map<DWORD, DWORD> m_mapGID_MarkID; // index = guild id

		std::set<DWORD> m_setFreeMarkID;
		std::string		m_pathPrefix;

	private:
		//
		// Symbol
		//
		std::map<DWORD, TGuildSymbol> m_mapSymbol;
};

#endif
