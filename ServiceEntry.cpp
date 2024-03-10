#include "StdInc.h"

#include <tchar.h>


SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE               g_ServiceStopEvent = INVALID_HANDLE_VALUE;

CKitSVC* g_pKitSVC = NULL;

VOID ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    g_ServiceStatus.dwCurrentState = dwCurrentState;
    g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        g_ServiceStatus.dwControlsAccepted = 0;
    else
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        g_ServiceStatus.dwCheckPoint = 0;
    else
        g_ServiceStatus.dwCheckPoint = dwCheckPoint++;

    if (dwCurrentState == SERVICE_STOPPED)
    {
        if (g_pKitSVC != NULL)
        {
            delete g_pKitSVC;
            g_pKitSVC = NULL;
        }
    }
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}


VOID ServiceStop()
{
    ServiceReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    SetEvent(g_ServiceStopEvent);
    CLogger::Instance()->Printf(LOG_SEVERITY_INFO, "Service stopped");
}

DWORD ServiceCtrlHandler(DWORD dwControl,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpContext)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        ServiceStop();
        break;
    case SERVICE_CONTROL_DEVICEEVENT:
        if (g_pKitSVC != 0)
            g_pKitSVC->OnDeviceEvent(dwEventType, lpEventData);
        break;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    default:
        CLogger::Instance()->Printf(LOG_SEVERITY_INFO, "Got unknown CtrlCode - %d", dwControl);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return NO_ERROR;
}

VOID ServiceWorkerThread()
{
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        Sleep(1000);
    }
}

VOID ServiceMain(DWORD argc, LPTSTR* argv)
{
    g_StatusHandle = RegisterServiceCtrlHandlerExW(_T("KIT"), ServiceCtrlHandler, NULL);

    if (g_StatusHandle == NULL)
    {
        ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;

    g_ServiceStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

    CLogger::Instance()->Printf(LOG_SEVERITY_INFO, "Service started");

    g_pKitSVC = new CKitSVC;
    if (RegisterDeviceNotificationW(g_StatusHandle, &NotificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE) == 0)
    {
        CLogger::Instance()->Printf(LOG_SEVERITY_CRITICAL, "Failed to register USB filter");
    }

    ServiceReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
    ServiceWorkerThread();
    ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc > 1) {
        CNotification::Instance()->ShowNotification(argv[1]);
        return 0;
    }

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(LPWSTR)L"KIT", (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL} 
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        return GetLastError();
    }

    return 0;
}
