#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>
#include <iostream>
#include <assert.h>
#include <wchar.h>
#include <WtsApi32.h>
#include <Userenv.h>
#include <string>
#include <cstring>
#include <dbt.h>

#include <setupapi.h>

#include <initguid.h>
#include <Usbiodef.h>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Ws2_32.lib")

#include "CLogger.h"
#include "CNotification.h"

#include "CKitSVC.h"
