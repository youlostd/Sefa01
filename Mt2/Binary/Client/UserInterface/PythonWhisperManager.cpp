#include "StdAfx.h"
#include "PythonWhisperManager.h"
#include "PythonChat.h"
#include "PythonPlayer.h"

CPythonWhisperManager::CPythonWhisperManager() : m_poGameWindow(NULL)
{
}

CPythonWhisperManager::~CPythonWhisperManager()
{
	Destroy();
}

void CPythonWhisperManager::Destroy()
{
	for (int i = 0; i < m_vec_WhisperDialogs.size(); ++i)
	{
		for (int j = 0; j < m_vec_WhisperDialogs[i].vec_kMessages.size(); ++j)
			delete[] m_vec_WhisperDialogs[i].vec_kMessages[j].szMessage;
	}

	m_vec_WhisperDialogs.clear();
}

void CPythonWhisperManager::Load()
{
	Destroy();

	if (m_stFileName.empty())
	{
		TraceError("cannot load whispers (no file name given)");
		return;
	}

	FILE* pFile = NULL;
	if (fopen_s(&pFile, m_stFileName.c_str(), "rb") != 0)
		return;

	int iDlgCount;
	if (!fread(&iDlgCount, sizeof(int), 1, pFile))
	{
		TraceError("cannot read whisper dialog count");
		fclose(pFile);
		return;
	}

	m_vec_WhisperDialogs.resize(iDlgCount);
	for (int i = 0; i < iDlgCount; ++i)
	{
		TWhisperDialog& rkDlg = m_vec_WhisperDialogs[i];

		if (!fread(&rkDlg.bMode, sizeof(BYTE), 1, pFile))
		{
			TraceError("cannot read dialog mode %d", i);
			fclose(pFile);
			return;
		}
		if (!fread(&rkDlg.bIsOpen, sizeof(bool), 1, pFile))
		{
			TraceError("cannot read is open %d", i);
			fclose(pFile);
			return;
		}
		if (!fread(&rkDlg.szName, CHARACTER_NAME_MAX_LEN, 1, pFile))
		{
			TraceError("cannot read character name %d", i);
			fclose(pFile);
			return;
		}
		rkDlg.szName[CHARACTER_NAME_MAX_LEN] = '\0';

		int iMsgCount;
		if (!fread(&iMsgCount, sizeof(int), 1, pFile))
		{
			TraceError("cannot read message count %d", i);
			fclose(pFile);
			return;
		}

		rkDlg.vec_kMessages.resize(iMsgCount);
		for (int j = 0; j < iMsgCount; ++j)
		{
			TWhisperMessage& rkMsg = rkDlg.vec_kMessages[j];
			if (!fread(&rkMsg.sMessageLen, sizeof(short), 1, pFile))
			{
				TraceError("cannot read message len dlg %d msg %d", i, j);
				fclose(pFile);
				return;
			}
			rkMsg.szMessage = new char[rkMsg.sMessageLen + 1];
			if (rkMsg.sMessageLen && !fread(rkMsg.szMessage, rkMsg.sMessageLen, 1, pFile))
			{
				TraceError("cannot read message dlg %d msg %d", i, j);
				fclose(pFile);
				return;
			}
			rkMsg.szMessage[rkMsg.sMessageLen] = '\0';

			PythonAddWhisper(rkDlg.szName, rkMsg.szMessage, rkDlg.bMode);
		}

		if (rkDlg.bIsOpen)
			PythonOpenWhisper(rkDlg.szName);
	}

	fclose(pFile);
}

void CPythonWhisperManager::Save()
{
	if (m_stFileName.empty())
	{
		TraceError("cannot load whispers (no file name given)");
		Destroy();
		return;
	}

	FILE* pFile = NULL;
	if (fopen_s(&pFile, m_stFileName.c_str(), "wb") != 0)
	{
		TraceError("cannot open file %s to write whispers", m_stFileName.c_str());
		Destroy();
		return;
	}

	int iDlgCount = m_vec_WhisperDialogs.size();
	fwrite(&iDlgCount, sizeof(int), 1, pFile);

	for (int i = 0; i < iDlgCount; ++i)
	{
		TWhisperDialog& rkDlg = m_vec_WhisperDialogs[i];

		fwrite(&rkDlg.bMode, sizeof(BYTE), 1, pFile);
		fwrite(&rkDlg.bIsOpen, sizeof(bool), 1, pFile);
		fwrite(rkDlg.szName, CHARACTER_NAME_MAX_LEN, 1, pFile);

		int iMsgCount = rkDlg.vec_kMessages.size();
		fwrite(&iMsgCount, sizeof(int), 1, pFile);

		for (int j = 0; j < iMsgCount; ++j)
		{
			TWhisperMessage& rkMsg = rkDlg.vec_kMessages[j];
			fwrite(&rkMsg.sMessageLen, sizeof(short), 1, pFile);
			fwrite(rkMsg.szMessage, rkMsg.sMessageLen, 1, pFile);
		}
	}

	fclose(pFile);
}

void CPythonWhisperManager::OnRecvWhisper(int iMode, const char* szNameFrom, const char* szMessage)
{
	TWhisperDialog* pDlg = __GetWhisperDialog(szNameFrom, iMode);

	static char s_szMessageBuf[CHAT_MAX_LEN + CHARACTER_NAME_MAX_LEN + 5];
	snprintf(s_szMessageBuf, sizeof(s_szMessageBuf), "%s : %s", szNameFrom, szMessage);

	__AddMessage(pDlg, s_szMessageBuf);
	Save();
}

void CPythonWhisperManager::OnSendWhisper(const char* szNameTo, const char* szMessage)
{
	TWhisperDialog* pDlg = __GetWhisperDialog(szNameTo);

	static char s_szMessageBuf[CHAT_MAX_LEN + CHARACTER_NAME_MAX_LEN + 5];
	snprintf(s_szMessageBuf, sizeof(s_szMessageBuf), "%s : %s", CPythonPlayer::Instance().GetName(), szMessage);

	__AddMessage(pDlg, s_szMessageBuf);
	Save();
}

void CPythonWhisperManager::OpenWhisper(const char* szName)
{
	for (int i = 0; i < m_vec_WhisperDialogs.size(); ++i)
	{
		if (!strcmp(m_vec_WhisperDialogs[i].szName, szName))
		{
			m_vec_WhisperDialogs[i].bIsOpen = true;
			Save();
			break;
		}
	}
}

void CPythonWhisperManager::CloseWhisper(const char* szName)
{
	for (int i = 0; i < m_vec_WhisperDialogs.size(); ++i)
	{
		if (!strcmp(m_vec_WhisperDialogs[i].szName, szName))
		{
			m_vec_WhisperDialogs[i].bIsOpen = false;
			Save();
			break;
		}
	}
}

void CPythonWhisperManager::ClearWhisper(const char* szName)
{
	for (int i = 0; i < m_vec_WhisperDialogs.size(); ++i)
	{
		if (!strcmp(m_vec_WhisperDialogs[i].szName, szName))
		{
			std::vector<TWhisperMessage>& rkVec = m_vec_WhisperDialogs[i].vec_kMessages;
			for (int i = 0; i < rkVec.size(); ++i)
				delete[] rkVec[i].szMessage;

			m_vec_WhisperDialogs.erase(m_vec_WhisperDialogs.begin() + i);

			Save();
			break;
		}
	}
}

void CPythonWhisperManager::PythonAddWhisper(const char* szOtherName, const char* szMessage, int iMode)
{
	if (iMode == -1)
		iMode = CPythonChat::WHISPER_TYPE_NORMAL;

	PyCallClassMemberFunc(m_poGameWindow, "LOAD_OnRecvWhisper", Py_BuildValue("(iss)", iMode, szOtherName, szMessage));
}

void CPythonWhisperManager::PythonOpenWhisper(const char* szOtherName)
{
	PyCallClassMemberFunc(m_poGameWindow, "LOAD_OnOpenWhisper", Py_BuildValue("(s)", szOtherName));
}

CPythonWhisperManager::TWhisperDialog* CPythonWhisperManager::__GetWhisperDialog(const char* szName, int iMode)
{
	if (iMode == -1)
		iMode = CPythonChat::WHISPER_TYPE_NORMAL;

	TWhisperDialog* pWhisperDialog = NULL;

	for (int i = 0; i < m_vec_WhisperDialogs.size(); ++i)
	{
		if (!strcmp(m_vec_WhisperDialogs[i].szName, szName))
		{
			pWhisperDialog = &m_vec_WhisperDialogs[i];
			break;
		}
	}

	if (!pWhisperDialog)
	{
		TWhisperDialog kDialog;
		m_vec_WhisperDialogs.push_back(kDialog);

		pWhisperDialog = &m_vec_WhisperDialogs[m_vec_WhisperDialogs.size() - 1];
		pWhisperDialog->bMode = iMode;
		pWhisperDialog->bIsOpen = false;
		strncpy(pWhisperDialog->szName, szName, sizeof(pWhisperDialog->szName) - 1);
	}
	else if (pWhisperDialog->bMode != CPythonChat::WHISPER_TYPE_GM && iMode == CPythonChat::WHISPER_TYPE_GM)
	{
		pWhisperDialog->bMode = CPythonChat::WHISPER_TYPE_GM;
	}

	return pWhisperDialog;
}

void CPythonWhisperManager::__AddMessage(TWhisperDialog* pDlg, const char* szMessage)
{
	TWhisperMessage kMessage;
	pDlg->vec_kMessages.push_back(kMessage);

	TWhisperMessage* pMsg = &pDlg->vec_kMessages[pDlg->vec_kMessages.size() - 1];
	pMsg->sMessageLen = strlen(szMessage);
	pMsg->szMessage = new char[pMsg->sMessageLen + 1];
	strncpy(pMsg->szMessage, szMessage, pMsg->sMessageLen);
	pMsg->szMessage[pMsg->sMessageLen] = '\0';
}
