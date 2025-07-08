#pragma once

#include "StdAfx.h"

class CPythonWhisperManager : public CSingleton<CPythonWhisperManager>
{
public:
	typedef struct SWhisperMessage {
		short	sMessageLen;
		char*	szMessage;
	} TWhisperMessage;
	typedef struct {
		BYTE							bMode;
		bool							bIsOpen;
		char							szName[CHARACTER_NAME_MAX_LEN + 1];
		std::vector<TWhisperMessage>	vec_kMessages;
	} TWhisperDialog;

public:
	CPythonWhisperManager();
	virtual ~CPythonWhisperManager();

	void		Destroy();

	void		SetGameWindow(PyObject* poGameWnd)	{ m_poGameWindow = poGameWnd; }

	void		SetFileName(const char* szFileName)	{ m_stFileName = szFileName; }
	const char*	GetFileName() const					{ return m_stFileName.c_str(); }

	void		Load();
	void		Save();

	void		OnRecvWhisper(int iMode, const char* szNameFrom, const char* szMessage);
	void		OnSendWhisper(const char* szNameTo, const char* szMessage);

	void		OpenWhisper(const char* szName);
	void		CloseWhisper(const char* szName);
	void		ClearWhisper(const char* szName);

	void		PythonAddWhisper(const char* szOtherName, const char* szMessage, int iMode = -1);
	void		PythonOpenWhisper(const char* szOtherName);

private:
	TWhisperDialog*	__GetWhisperDialog(const char* szName, int iMode = -1);
	void			__AddMessage(TWhisperDialog* pDlg, const char* szMessage);

private:
	PyObject*					m_poGameWindow;
	std::string					m_stFileName;
	std::vector<TWhisperDialog>	m_vec_WhisperDialogs;
};
