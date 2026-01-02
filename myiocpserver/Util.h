#pragma once

namespace Util
{
	bool TryGetProcessName(TCHAR processName[MAX_PATH]);

	const char* WSAError(int errorcode);
}