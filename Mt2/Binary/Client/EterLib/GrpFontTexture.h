#pragma once

#include "GrpTexture.h"
#include "GrpImageTexture.h"
#include "GrpDIB.h"

#include <vector>
#include <map>

class CGraphicFontTexture : public CGraphicTexture
{
public:
	typedef pair<WORD,wchar_t> TCharacterKey;

	typedef struct SCharacterInfomation
	{
		short index;
		short width;
		short height;
		float left;
		float top;
		float right;
		float bottom;
		float advance;
	} TCharacterInfomation;

	typedef std::vector<TCharacterInfomation*>		TPCharacterInfomationVector;

public:
	CGraphicFontTexture();
	virtual ~CGraphicFontTexture();

	void Destroy();
	bool Create(const char* c_szFontName, int fontSize, bool bItalic, bool bIsBold);

	bool CreateDeviceObjects();
	void DestroyDeviceObjects();

	bool CheckTextureIndex(DWORD dwTexture);
	void SelectTexture(DWORD dwTexture);

	bool UpdateTexture();

	TCharacterInfomation* GetCharacterInfomation(WORD codePage, wchar_t keyValue);
	TCharacterInfomation* UpdateCharacterInfomation(TCharacterKey code);

	bool IsEmpty() const;

	void GetTextureSize(float& width, float& height) { width = float(m_dib.GetWidth()); height = float(m_dib.GetHeight()); }

protected:
	void Initialize();

	bool AppendTexture();

	HFONT GetFont(WORD codePage);

protected:		
	typedef std::vector<CGraphicImageTexture*>				TGraphicImageTexturePointerVector;
	typedef std::map<TCharacterKey, TCharacterInfomation>	TCharacterInfomationMap;

	typedef struct SFontInfoData {
		WORD	wCodePage;
		int		iFontSize;
		bool	bIsItalic;
		bool	bIsBold;

		bool operator<(SFontInfoData const& o) const
		{
			if (wCodePage != o.wCodePage)
				return wCodePage < o.wCodePage;
			if (iFontSize != o.iFontSize)
				return iFontSize < o.iFontSize;
			if (bIsItalic != o.bIsItalic)
				return !bIsItalic;
			return bIsBold < o.bIsBold;
		}
	} TFontInfoData;
	typedef std::map<TFontInfoData, HFONT> TFontMap;

protected:		
	CGraphicDib	m_dib;

	HFONT	m_hFontOld;
	HFONT	m_hFont;

	TGraphicImageTexturePointerVector m_pFontTextureVector;

	TCharacterInfomationMap m_charInfoMap;

	TFontMap m_fontMap;

	int m_x;
	int m_y;
	int m_step;		
	bool m_isDirty;

	TCHAR	m_fontName[LF_FACESIZE];
	long	m_fontSize;
	bool	m_bItalic;
	bool	m_bIsBold;
};
