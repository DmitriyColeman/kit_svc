#include "StdInc.h"

CKitSVC::CKitSVC()
{
    DWORD size = sizeof(m_szComputerName) / sizeof(m_szComputerName[0]);
    //GetComputerNameA(szComputerName, &size);
    strcpy_s(m_szComputerName, "R112-MAIN");

    std::string strComputerName(m_szComputerName);
    ParseComputerName(strComputerName);

    if (m_iCabinetNumber == 0)
    {
        // FAIL!
        CLogger::Instance()->Printf(LOG_SEVERITY_CRITICAL, "Failed to retrive computer location info: %s", strComputerName.c_str());
        return;
    }

    CLogger::Instance()->Printf(LOG_SEVERITY_INFO, "Computer location info: cabinet: %d, number: %d, MAIN: %d", m_iCabinetNumber, m_iPCNumber, m_bIsMain);

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);;
    if (iResult != 0)
    {
        CLogger::Instance()->Printf(LOG_SEVERITY_CRITICAL, "WSAStartup failed - 0x%08X", iResult);
        return;
    }
    m_pSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    ZeroMemory(&m_pServerAddr, sizeof(m_pServerAddr));
    m_pServerAddr.sin_family = AF_INET;
    m_pServerAddr.sin_port = htons(3005);
    inet_pton(AF_INET, "127.0.0.1", &m_pServerAddr.sin_addr);

    if (m_bIsMain)
    {
        CreateThread(0, 0, CKitSVC::ServerThread, 0, 0, 0);
    }
}

void CKitSVC::SendMessage(const SKitMessage& message) 
{
    char* szData = (char*)&message;
    for (int i = 4; i < sizeof(message); i++)
    {
        szData[i] ^= 0x21;
    }

    int iResult = sendto(m_pSocket, szData, sizeof(message), 0,
        (SOCKADDR*)&m_pServerAddr, sizeof(m_pServerAddr));
}

DWORD CKitSVC::ServerThread(LPVOID lpParameter)
{

    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    int iResult = getaddrinfo(NULL, "3005", &hints, &result);
    if (iResult != 0) {
        CLogger::Instance()->Printf(LOG_SEVERITY_CRITICAL, "[SERVER] getaddrinfo failed - 0x%08X", iResult);
        WSACleanup();
        return E_FAIL;
    }

    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        CLogger::Instance()->Printf(LOG_SEVERITY_CRITICAL, "[SERVER] Error at socket() - 0x%08X", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return E_FAIL;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        CLogger::Instance()->Printf(LOG_SEVERITY_CRITICAL, "[SERVER] bind failed with error - 0x%08X", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return E_FAIL;
    }
    freeaddrinfo(result);

    char recvbuf[512];
    int recvbuflen = 512;
    sockaddr_in sender_addr;
    int sender_addr_size = sizeof(sender_addr);

    while (1)
    {
        iResult = recvfrom(ListenSocket, recvbuf, recvbuflen, 0, (SOCKADDR*)&sender_addr, &sender_addr_size);
        if (iResult > 0) 
        {
            if (iResult == sizeof(SKitMessage))
            {
                SKitMessage* message = reinterpret_cast<SKitMessage*>(recvbuf);
                if (message->dwSignature == 'DTIK')
                {
                    // OK, lets decrypt message
                    for (int i = 4; i < sizeof(SKitMessage); i++)
                    {
                        recvbuf[i] ^= 0x21;
                    }
                    // If our signature is same
                    if (message->dwEncSignature == 'ETIK' && message->szInfoType > INFO_TYPE_ERROR_DROP_ME && message->szInfoType < INFO_TYPE_MAX)
                    {
                        // Read report info
                        char szPcName[17];
                        memcpy(szPcName, message->szComputerName, 16);
                        szPcName[16] = '\0';

                        wchar_t szReport[512];
                        bool bGot = true;
                        memset(szReport, 0, 512);
                        switch (message->szInfoType)
                        {
                        case INFO_TYPE_PEREPHERY_INSERTED:
                            swprintf_s(szReport, L"На компьютере %hs была вставлена переферия!", szPcName);
                            break;
                        default:
                            bGot = false;
                            break;
                        }

                        if (bGot)
                        {
                            CNotification::Instance()->SendNotificationNow(szReport);
                        }
                    }
                }
            }
        }
    }
    return ERROR_SUCCESS;
}

void CKitSVC::ParseComputerName(const std::string& computerName) {

    m_iCabinetNumber = 0;
    m_iPCNumber = 0;
    m_bIsMain = false;

    size_t dashPos = computerName.find('-');
    if (dashPos != std::string::npos) {
        m_iCabinetNumber = std::stoi(computerName.substr(1, dashPos - 1));

        std::string pcNumberStr = computerName.substr(dashPos + 1);
        if (pcNumberStr == "MAIN") {
            m_iPCNumber = 99;
            m_bIsMain = true;
        }
        else {
            m_iPCNumber = std::stoi(pcNumberStr);
        }
    }
}

void CKitSVC::OnDeviceEvent(DWORD dwEventType, LPVOID lpEventData)
{
    switch (dwEventType) {
        case DBT_DEVICEARRIVAL: {
            auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lpEventData);
            if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                auto pDevInf = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lpEventData);
                SKitMessage msg;
                msg.szInfoType = INFO_TYPE_PEREPHERY_INSERTED;
                memset(msg.szComputerName, 0, 16);
                memcpy(msg.szComputerName, m_szComputerName, 16);
                SendMessage(msg);
            }
            break;
        }
        case DBT_DEVICEREMOVECOMPLETE: {
            auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lpEventData);
            if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                auto pDevInf = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lpEventData);
               
            }
            break;
        }
    }
}