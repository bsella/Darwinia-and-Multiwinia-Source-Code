#include <string>
#if defined(TARGET_MSVC)
#include <mmsystem.h>
#include <dxdiag.h>
#endif

#if defined(TARGET_OS_LINUX)
#include <alsa/asoundlib.h>
#endif

#include <stdio.h>
#include <locale>

#include "lib/system_info.h"


SystemInfo *g_systemInfo = NULL;


SystemInfo::SystemInfo()
{
	GetLocaleDetails();
	GetAudioDetails();
//	GetDirectXVersion();
}

void SystemInfo::GetLocaleDetails()
{
#if defined(TARGET_MSVC)
	int size;
	bool languageSuccess = false;

	if( !languageSuccess )
	{
		size = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, NULL, 0);
		m_localeInfo.m_language.resize(size + 1);
		DarwiniaReleaseAssert(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, m_localeInfo.m_language.data(), size),
					  "Couldn't get locale details");
	}
#else
	m_localeInfo.m_language= std::locale("").name();
#endif
}


void SystemInfo::GetAudioDetails()
{
	int bestScore = -1000;
#if defined(TARGET_MSVC)
	unsigned int numDevs = waveOutGetNumDevs();
	m_audioInfo.m_numDevices = numDevs;
	m_audioInfo.m_preferredDevice = -1;

	m_audioInfo.m_deviceNames = new char *[numDevs];
	for (unsigned int i = 0; i < numDevs; ++i)
	{
		WAVEOUTCAPS caps;
		waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS));
		m_audioInfo.m_deviceNames[i] = strdup(caps.szPname);

		int score = 0;
		if (caps.dwFormats & WAVE_FORMAT_1M08) score++;
		if (caps.dwFormats & WAVE_FORMAT_1M16) score++;
		if (caps.dwFormats & WAVE_FORMAT_1S08) score++;
		if (caps.dwFormats & WAVE_FORMAT_1S16) score++;
		if (caps.dwFormats & WAVE_FORMAT_2M08) score++;
		if (caps.dwFormats & WAVE_FORMAT_2M16) score++;
		if (caps.dwFormats & WAVE_FORMAT_2S08) score++;
		if (caps.dwFormats & WAVE_FORMAT_2S16) score++;
		if (caps.dwFormats & WAVE_FORMAT_4M08) score++;
		if (caps.dwFormats & WAVE_FORMAT_4M16) score++;
		if (caps.dwFormats & WAVE_FORMAT_4S08) score++;
		if (caps.dwFormats & WAVE_FORMAT_4S16) score++;
		if (caps.dwSupport & WAVECAPS_SYNC) score -= 10;
		if (caps.dwSupport & WAVECAPS_LRVOLUME) score++;
		if (caps.dwSupport & WAVECAPS_PITCH) score++;
		if (caps.dwSupport & WAVECAPS_PLAYBACKRATE) score++;
		if (caps.dwSupport & WAVECAPS_VOLUME) score++;
		if (caps.dwSupport & WAVECAPS_SAMPLEACCURATE) score++;

		if (score > bestScore)
		{
			m_audioInfo.m_preferredDevice = i;
			bestScore = score;
		}
	}

	DarwiniaReleaseAssert(m_audioInfo.m_preferredDevice != -1, "No suitable audio hardware found");
#endif

#if defined(TARGET_OS_LINUX)

	int card_index = -1;

	while(snd_card_next(&card_index), card_index != -1)
	{
		char* device_name;
		snd_card_get_name(card_index, &device_name);

		m_audioInfo.m_deviceNames.emplace_back(device_name);

		{
			int score = 0;

			{
				snd_ctl_t *ctl;
				
				{
					std::string ctl_name;
					ctl_name += "hw:" + std::to_string(card_index);
					
					snd_ctl_open(&ctl, ctl_name.c_str(), 0);
				}

				int device_index = -1;

				while(snd_ctl_pcm_next_device(ctl, &device_index), device_index != -1)
				{
					snd_pcm_t *pcm;

					{
						std::string device_string;
						device_string += "hw:" + std::to_string(card_index) + "," + std::to_string(device_index);

						if(snd_pcm_open(&pcm, device_string.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) != 0)
						{
							continue;
						}
					}

					{
						snd_pcm_hw_params_t* hw_params;
						snd_pcm_hw_params_malloc(&hw_params);
						snd_pcm_hw_params_any(pcm, hw_params);

						if(snd_pcm_hw_params_test_rate(pcm, hw_params, 11025, 0) == 0) score++;
						if(snd_pcm_hw_params_test_rate(pcm, hw_params, 22050, 0) == 0) score++;
						if(snd_pcm_hw_params_test_rate(pcm, hw_params, 44100, 0) == 0) score++;

						snd_pcm_hw_params_free(hw_params);
					}

					snd_pcm_close(pcm);
				}

				snd_ctl_close(ctl);
			}

			if (score > bestScore)
			{
				m_audioInfo.m_preferredDevice = device_name;
				bestScore = score;
			}
		}
	}

#endif
}


void SystemInfo::GetDirectXVersion()
{
#if defined(TARGET_MSVC)
	HKEY hkey;
	long errCode;
	unsigned long bufLen = 256;
	unsigned char buf[256];

	m_directXVersion = -1;

	errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\DirectX", 0, KEY_READ, &hkey);
	DarwiniaReleaseAssert(errCode == ERROR_SUCCESS, "Failed to get DirectX Version");
	errCode = RegQueryValueEx(hkey, "InstalledVersion", NULL, NULL, buf, &bufLen);

    if( errCode == ERROR_SUCCESS )
    {
		m_directXVersion = buf[3];
	}
	else
	{
		// NOTE by Chris : This value doesn't exist on Windows98
		// However the key "Version" does exist
		errCode = RegQueryValueEx(hkey, "Version", NULL, NULL, buf, &bufLen );
		if( errCode == ERROR_SUCCESS )
		{
			m_directXVersion = buf[3];
		}
    }

	RegCloseKey(hkey);

//	if (m_directXVersion == -1)
//	{
//		long hr;
//		bool bCleanupCOM = false;
//		bool bSuccessGettingMajor = false;
//
//		// Init COM.  COM may fail if its already been inited with a different
//		// concurrency model.  And if it fails you shouldn't release it.
//		hr = CoInitialize(NULL);
//		bCleanupCOM = SUCCEEDED(hr);
//
//		// Get an IDxDiagProvider
//		bool bGotDirectXVersion = false;
//		IDxDiagProvider* pDxDiagProvider = NULL;
//		hr = CoCreateInstance( CLSID_DxDiagProvider,
//							   NULL,
//							   CLSCTX_INPROC_SERVER,
//							   IID_IDxDiagProvider,
//							   (LPVOID*) &pDxDiagProvider );
//		if( SUCCEEDED(hr) )
//		{
//			// Fill out a DXDIAG_INIT_PARAMS struct
//			DXDIAG_INIT_PARAMS dxDiagInitParam;
//			ZeroMemory( &dxDiagInitParam, sizeof(DXDIAG_INIT_PARAMS) );
//			dxDiagInitParam.dwSize                  = sizeof(DXDIAG_INIT_PARAMS);
//			dxDiagInitParam.dwDxDiagHeaderVersion   = DXDIAG_DX9_SDK_VERSION;
//			dxDiagInitParam.bAllowWHQLChecks        = false;
//			dxDiagInitParam.pReserved               = NULL;
//
//			// Init the m_pDxDiagProvider
//			hr = pDxDiagProvider->Initialize( &dxDiagInitParam );
//			if( SUCCEEDED(hr) )
//			{
//				IDxDiagContainer* pDxDiagRoot = NULL;
//				IDxDiagContainer* pDxDiagSystemInfo = NULL;
//
//				// Get the DxDiag root container
//				hr = pDxDiagProvider->GetRootContainer( &pDxDiagRoot );
//				if( SUCCEEDED(hr) )
//				{
//					// Get the object called DxDiag_SystemInfo
//					hr = pDxDiagRoot->GetChildContainer( L"DxDiag_SystemInfo", &pDxDiagSystemInfo );
//					if( SUCCEEDED(hr) )
//					{
//						VARIANT var;
//						VariantInit( &var );
//
//						// Get the "dwDirectXVersionMajor" property
//						hr = pDxDiagSystemInfo->GetProp( L"dwDirectXVersionMajor", &var );
//						if( SUCCEEDED(hr) && var.vt == VT_UI4 )
//						{
//							m_directXVersion = var.ulVal;
//							bSuccessGettingMajor = true;
//						}
//						VariantClear( &var );
//
//						// If it all worked right, then mark it down
//						if( bSuccessGettingMajor )
//							bGotDirectXVersion = true;
//
//						pDxDiagSystemInfo->Release();
//					}
//
//					pDxDiagRoot->Release();
//				}
//			}
//
//			pDxDiagProvider->Release();
//		}
//
//		if( bCleanupCOM )
//			CoUninitialize();
//	}
#endif
}
