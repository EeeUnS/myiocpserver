#include "pch.h"
#include "Util.h"

namespace Util
{
	bool TryGetProcessName(TCHAR processName[MAX_PATH])
	{
		TCHAR processPath[MAX_PATH] = { 0 };
		DWORD error = GetModuleFileName(NULL, processPath, MAX_PATH);
		if (error == 0 || error == MAX_PATH)
		{
			lstrcpy(processName, TEXT("UnknownProcess"));
			return false;
		}

		// Extract just the filename (without path)
		TCHAR* pProcessName = processPath;
		for (TCHAR* p = processPath; *p; ++p)
		{
			if (*p == '\\' || *p == '/')
				pProcessName = p + 1;
		}

		lstrcpy(processName, pProcessName);
		return true;
	}
}
