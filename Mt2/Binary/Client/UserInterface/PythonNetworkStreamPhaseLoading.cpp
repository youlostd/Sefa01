#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "PythonApplication.h"
#include "NetworkActorManager.h"

#include "AbstractPlayer.h"

#include "../eterPack/EterPackManager.h"

using namespace network;

bool CPythonNetworkStream::LoadConvertTable(DWORD dwEmpireID, const char* c_szFileName)
{
	if (dwEmpireID<1 || dwEmpireID>=4)
		return false;

	CMappedFile file;
	const VOID* pvData;
	if (!CEterPackManager::Instance().Get(file, c_szFileName, &pvData))
		return false;

	DWORD dwEngCount=26;
	DWORD dwHanCount=(0xc8-0xb0+1)*(0xfe-0xa1+1);
	DWORD dwHanSize=dwHanCount*2;
	DWORD dwFileSize=dwEngCount*2+dwHanSize;

	if (file.Size()<dwFileSize)
		return false;

	char* pcData=(char*)pvData;

	STextConvertTable& rkTextConvTable=m_aTextConvTable[dwEmpireID-1];
	memcpy(rkTextConvTable.acUpper, pcData, dwEngCount);pcData+=dwEngCount;
	memcpy(rkTextConvTable.acLower, pcData, dwEngCount);pcData+=dwEngCount;
	memcpy(rkTextConvTable.aacHan, pcData, dwHanSize);

	return true;
}

// Loading ---------------------------------------------------------------------------
void CPythonNetworkStream::LoadingPhase()
{
	InputPacket packet;
	bool ret;

	while (Recv(packet))
	{
		switch (packet.get_header<TGCHeader>())
		{
			case TGCHeader::MAIN_CHARACTER:
				RecvMainCharacter(packet.get<GCMainCharacterPacket>());
				break;

			default:
				GamePhasePacket(packet, ret);
				break;
		}

		if (packet.get_header<TGCHeader>() == TGCHeader::PHASE)
			break;
	}
}

void CPythonNetworkStream::SetLoadingPhase()
{
	if ("Loading"!=m_strPhase)
		m_phaseLeaveFunc.Run();

	Tracen("");
	Tracen("## Network - Loading Phase ##");
	Tracen("");

	m_strPhase = "Loading";

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::LoadingPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveLoadingPhase);

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.Clear();

#ifdef ENABLE_GUILD_SAFEBOX
	CPythonSafeBox& rkSafeBox = CPythonSafeBox::Instance();
	rkSafeBox.SetGuildEnable(false);
	rkSafeBox.ClearGuildLogInfo();
#endif

#ifdef ENABLE_AUCTION
	CPythonAuction& rkAuction = CPythonAuction::Instance();
	rkAuction.initialize();
#endif

#ifdef ENABLE_RUNE_SYSTEM
	CPythonRune& rkRune = CPythonRune::Instance();
	rkRune.Clear();
#endif

#ifdef ENABLE_PET_ADVANCED
	CPythonPetAdvanced& rkPet = CPythonPetAdvanced::Instance();
	rkPet.Clear();
#endif

	CFlyingManager::Instance().DeleteAllInstances();
	CEffectManager::Instance().DeleteAllInstances();

	__DirectEnterMode_Initialize();
}

bool CPythonNetworkStream::RecvMainCharacter(std::unique_ptr<GCMainCharacterPacket> pack)
{
	m_dwMainActorVID = pack->vid();
	m_dwMainActorRace = pack->race_num();
	m_dwMainActorEmpire = pack->empire();
	m_dwMainActorSkillGroup = pack->skill_group();

	m_rokNetActorMgr->SetMainActorVID(m_dwMainActorVID);

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.SetName(pack->chr_name().c_str());
	rkPlayer.SetMainCharacterIndex(GetMainActorVID());

	if (!pack->bgm_name().empty())
	{
		if (pack->bgm_vol() > 0)
			__SetFieldMusicFileInfo(pack->bgm_name().c_str(), pack->bgm_vol());
		else
			__SetFieldMusicFileName(pack->bgm_name().c_str());
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", pack->x(), pack->y()));

	SendClientVersionPacket();
	return true;
}

static std::string	gs_fieldMusic_fileName;
static float		gs_fieldMusic_volume = 1.0f / 5.0f * 0.1f;

void CPythonNetworkStream::__SetFieldMusicFileName(const char* musicName)
{
	gs_fieldMusic_fileName = musicName;
}

void CPythonNetworkStream::__SetFieldMusicFileInfo(const char* musicName, float vol)
{
	gs_fieldMusic_fileName = musicName;
	gs_fieldMusic_volume = vol;
}

const char* CPythonNetworkStream::GetFieldMusicFileName()
{
	return gs_fieldMusic_fileName.c_str();
}

float CPythonNetworkStream::GetFieldMusicVolume()
{
	return gs_fieldMusic_volume;
}
// END_OF_SUPPORT_BGM


bool CPythonNetworkStream::__RecvPlayerPoints(std::unique_ptr<GCPointsPacket> pack)
{
	for (DWORD i = 0; i < POINT_MAX_NUM; ++i)
		CPythonPlayer::Instance().SetStatus(i, pack->points(i));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshStatus", Py_BuildValue("()"));
	return true;
}

void CPythonNetworkStream::StartGame()
{
	m_isStartGame=TRUE;
}

bool CPythonNetworkStream::SendEnterGame()
{
	if (!SendFlush(TCGHeader::ENTERGAME))
	{
		Tracen("Send EnterGamePacket");
		return false;
	}

	return true;
}
