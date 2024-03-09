#pragma once

enum ELogSeverity {
	LOG_SEVERITY_DEBUG,
	LOG_SEVERITY_INFO,
	LOG_SEVERITY_WARNING,
	LOG_SEVERITY_ERROR,
	LOG_SEVERITY_CRITICAL,

	LOG_SEVERITY_MAX,
};

class CLogger {
public:
	static CLogger* Instance();
	CLogger() {
		bool bWasTruncated = false;
		m_hStdOutput = OpenLog(bWasTruncated);
		m_hMutex = CreateMutexW(NULL, FALSE, NULL);
		GetCriticalHandle(); // Perform create operation on start

		if (bWasTruncated)
		{
			Printf(LOG_SEVERITY_WARNING, "Log cleared (size limit extended)");
		}
	}

	void Printf(ELogSeverity eSeverity, const char* szText, ...);

	bool SetMinimumLogLevel(ELogSeverity eNewSeverity)
	{
		if (eNewSeverity < 0 || eNewSeverity >= LOG_SEVERITY_MAX)
			return false;

		m_cLogSeverity = eNewSeverity;
		return true;
	}

	HANDLE OpenLog(bool &bWasTruncated)
	{
		wchar_t buffer[MAX_PATH];
		if (GetModuleFileNameW(NULL, buffer, MAX_PATH) == 0) {
			return 0;
		}

		std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
		std::wstring strDirectory = std::wstring(buffer).substr(0, pos + 1) + L"\\Logs\\";
		CreateDirectoryW(strDirectory.c_str(), 0);
		m_strDirectory = strDirectory;
		HANDLE hFile = CreateFileW((strDirectory + L"\\main.log").c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
		if (!hFile) return 0;

		LARGE_INTEGER liSize;
		GetFileSizeEx(hFile, &liSize);
		if (liSize.QuadPart > (1024 * 1024 * 1024 * 5)) // 5 GB of logs is MAXIMUM
		{
			bWasTruncated = true;
			CloseHandle(hFile);
			return CreateFileW((strDirectory + L"\\main.log").c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
		}
		SetFilePointer(hFile, 0, 0, FILE_END);
		return hFile;
	}

	HANDLE GetCriticalHandle()
	{
		if (m_hCriticalLog != 0)
			return m_hCriticalLog;
		HANDLE hFile = CreateFileW((m_strDirectory + L"\\critical.log").c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL);
		if (!hFile) return 0;
		m_hCriticalLog = hFile;
		SetFilePointer(hFile, 0, 0, FILE_END);
		return hFile;
	}
private:
	const char* GetLogSeverityText(ELogSeverity eSeverity)
	{
		switch (eSeverity)
		{
		case LOG_SEVERITY_DEBUG:
			return "debug";
		case LOG_SEVERITY_INFO:
			return "info";
		case LOG_SEVERITY_WARNING:
			return "warning";
		case LOG_SEVERITY_ERROR:
			return "error";
		case LOG_SEVERITY_CRITICAL:
			return "critical";
		}
		return "unknown";
	}

#ifdef DEBUG
	char m_cLogSeverity = LOG_SEVERITY_DEBUG;
#else 
	char m_cLogSeverity = LOG_SEVERITY_INFO;
#endif
	HANDLE m_hStdOutput = NULL;
	HANDLE m_hCriticalLog = NULL;
	HANDLE m_hMutex = NULL;
	std::wstring m_strDirectory;

	static CLogger* m_pInstance;

#ifndef MM_CLI
	HANDLE m_hPipe = 0;
#endif
};


