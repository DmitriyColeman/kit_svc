#pragma once
class CNotification
{
public:
	static CNotification* Instance();
    void SendNotificationNow(const wchar_t* message);

    void ShowNotification(const wchar_t* message);
private:
    bool GetSessionUserToken(DWORD sessionId, HANDLE& hToken) {
        if (!WTSQueryUserToken(sessionId, &hToken)) {
            return false;
        }
        return true;
    }

    bool CreateProcessInUserSession(const HANDLE& hToken, const wchar_t* appPath, const wchar_t* args) {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        si.cb = sizeof(STARTUPINFO);
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        void* lpEnvironment = NULL;
        if (!CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE)) {
            CLogger::Instance()->Printf(LOG_SEVERITY_ERROR, "Failed to create environment block - %08X", GetLastError());
            return false;
        }
        if (CreateProcessAsUserW(hToken, appPath, (LPWSTR)args, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, lpEnvironment, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            DestroyEnvironmentBlock(lpEnvironment);
            return true;
        }

        DestroyEnvironmentBlock(lpEnvironment);
        CLogger::Instance()->Printf(LOG_SEVERITY_ERROR, "Failed to start notification process - %08X", GetLastError());
        return false;
    }

    std::wstring GetServiceExecutablePath() {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
            return L"";
        }
        return std::wstring(path);
    }


	static CNotification* m_pInstance;
};

