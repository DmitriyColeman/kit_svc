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
};

