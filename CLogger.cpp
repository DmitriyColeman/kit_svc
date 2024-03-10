#include "StdInc.h"

CLogger* CLogger::m_pInstance = nullptr;
CLogger* CLogger::Instance()
{
	if (m_pInstance == nullptr)
		m_pInstance = new CLogger;
	return m_pInstance;
}

void CLogger::Printf(ELogSeverity eSeverity, const char* szText, ...)
{
	assert(m_cLogSeverity >= 0 && m_cLogSeverity < LOG_SEVERITY_MAX);
	
	if (eSeverity < m_cLogSeverity)
		return;
	WaitForSingleObject(m_hMutex, INFINITE);

	va_list vl;
	va_start(vl, szText);

	// temp buffer
	char buffer[1024];
	__try {
		vsnprintf(buffer, sizeof(buffer), szText, vl);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		strcpy_s(buffer, "Format exception");
	}

	va_end(vl);
	char szLog[2048];
	DWORD dwWritten;

	SYSTEMTIME s;
	GetLocalTime(&s);
	sprintf_s(szLog, "[%02d-%02d-%02d %02d:%02d:%02d] [%s]: %s\r\n", s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond, 
		GetLogSeverityText(eSeverity), buffer);
	if (eSeverity != LOG_SEVERITY_CRITICAL)
	{
		WriteFile(m_hStdOutput, szLog, strlen(szLog), &dwWritten, NULL);
	}
	else {
		WriteFile(GetCriticalHandle(), szLog, strlen(szLog), &dwWritten, NULL);
	}

	ReleaseMutex(m_hMutex);
}
