#pragma once

enum EInfoType {
	INFO_TYPE_ERROR_DROP_ME,
	INFO_TYPE_PEREPHERY_INSERTED,
	INFO_TYPE_PEREPHERY_REMOVED,
	
	INFO_TYPE_MAX
};

struct SKitMessage {
	// HEADER - KITD
	DWORD dwSignature = 'DTIK';
	// ENCRYPTED
	// ENCRYPTED HEADER
	DWORD dwEncSignature = 'ETIK';
	SYSTEMTIME pSystemTime;
	char szComputerName[16];

	char szInfoType = INFO_TYPE_ERROR_DROP_ME;

};

class CKitSVC
{
public:
	CKitSVC();
	// NETWORK
	static DWORD WINAPI ServerThread(LPVOID lpParameter);
	void SendMessage(const SKitMessage& message);

	// USB WATCHER
	void OnDeviceEvent(DWORD dwEventType, LPVOID lpEventData);
private:
	char m_szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	int m_iCabinetNumber;
	int m_iPCNumber;
	bool m_bIsMain;

	SOCKET m_pSocket;
	sockaddr_in m_pServerAddr;

	void ParseComputerName(const std::string& computerName);
	void GetMainPCIP()
	{
		char szNetBIOS[16];
		sprintf_s(szNetBIOS, 16, "R%d-MAIN", m_iCabinetNumber);
		if (!ResolveHostNameToIP(szNetBIOS))
		{
			CLogger::Instance()->Printf(LOG_SEVERITY_WARNING, "Failed to resolve %s to ip address. Trying DNS name", szNetBIOS);
			char szDomain[128];
			sprintf_s(szDomain, 128, "R%d-MAIN.kit.domain", m_iCabinetNumber);
			if (!ResolveHostNameToIP(szDomain))
			{
				CLogger::Instance()->Printf(LOG_SEVERITY_WARNING, "Failed to resolve %s to ip address.", szDomain);
			}
		}
	}

	bool ResolveHostNameToIP(const char* hostname) {
		struct addrinfo hints, * res;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET; 
		hints.ai_socktype = SOCK_STREAM;

		if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
			return false;
		}

		m_pServerAddr = *((struct sockaddr_in*)res->ai_addr);
		freeaddrinfo(res);

		return true;
	}
};

