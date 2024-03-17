#include "StdInc.h"

CNotification* CNotification::m_pInstance = nullptr;
CNotification* CNotification::Instance()
{
	if (m_pInstance == nullptr)
		m_pInstance = new CNotification;
	return m_pInstance;
}

void CNotification::SendNotificationNow(const wchar_t* message) {
    PWTS_SESSION_INFOW pSessionInfo = NULL;
    DWORD sessionCount = 0;

    if (WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &sessionCount)) {
        std::wstring exePath = GetServiceExecutablePath();
        for (DWORD i = 0; i < sessionCount; ++i) {
            if (pSessionInfo[i].State == WTSActive) {
                HANDLE hToken = NULL;
                if (GetSessionUserToken(pSessionInfo[i].SessionId, hToken)) {
                    wchar_t args[1024+MAX_PATH];
                    swprintf(args, 1024+MAX_PATH, L"%s \"%s\"", exePath.c_str(), message);
                    CreateProcessInUserSession(hToken, exePath.c_str(), args);
                    CloseHandle(hToken);
                }
            }
        }
        WTSFreeMemory(pSessionInfo);
    }
}

// Used for session display
void CNotification::ShowNotification(const wchar_t* message)
{
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = NULL;
    nid.uID = 1; 
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Уведомление"); 
    wcscpy_s(nid.szInfo, message); 
    wcscpy_s(nid.szInfoTitle, L"СПБКИТ"); 
    nid.uTimeout = 10000;
    nid.hIcon = LoadIcon(NULL, IDI_WARNING);

    Shell_NotifyIconW(NIM_ADD, &nid);
    Sleep(8000);

    Shell_NotifyIconW(NIM_DELETE, &nid);
}
