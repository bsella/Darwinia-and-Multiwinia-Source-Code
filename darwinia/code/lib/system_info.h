#ifndef INCLUDED_SYSTEM_INFO_H
#define INCLUDED_SYSTEM_INFO_H
#include <string>
#include <vector>

//*****************************************************************************
// Class LocaleInfo
//*****************************************************************************

class LocaleInfo
{
public:
	std::string m_language;
};



//*****************************************************************************
// Class AudioInfo
//*****************************************************************************

class AudioInfo
{
public:
	std::vector<std::string> m_deviceNames;
	std::string_view m_preferredDevice;
};



//*****************************************************************************
// Class SystemInfo
//*****************************************************************************

class SystemInfo
{
private:
	void GetLocaleDetails();
	void GetAudioDetails();
	void GetDirectXVersion();

public:
	LocaleInfo	m_localeInfo;
	AudioInfo	m_audioInfo;
	int			m_directXVersion;

	SystemInfo();
};


extern SystemInfo *g_systemInfo;


#endif
