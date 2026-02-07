#include "pch.h"
#include "Logger.h"
#include "DebugHelper.h"
#include <cstdio>
#include <iostream>


constexpr const char* LOG_LEVEL_NAME_TABLE[] = {
#define X(name) #name,
	LOG_LEVEL_LIST
#undef X
};

constexpr const char* GetLogLevelName(ELogLevel eLevel)
{
	int nIndex = static_cast<int>(eLevel);
	ASSERT(nIndex < static_cast<int>(ELogLevel::Count));

	return (nIndex < static_cast<int>(ELogLevel::Count)) ? LOG_LEVEL_NAME_TABLE[nIndex] : "Unknown";
}


constexpr size_t MAX_LOG_BUFFER_SIZE = 4096;


CLogger::CLogger()
	: m_hQueueEvent(NULL)
	, m_bRunning(false)
{
	for (int i = 0; i < static_cast<int>(ELogLevel::Count); ++i)
	{
		m_wCurrentDay[i] = 0;
	}
	
	m_hQueueEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(m_hQueueEvent != NULL);

	m_vecWriteBuffer.reserve(64);
	m_vecReadBuffer.reserve(64);
}

CLogger::~CLogger()
{
	Stop();
	
	ASSERT(m_hQueueEvent != NULL);
	::CloseHandle(m_hQueueEvent);
	m_hQueueEvent = NULL;
	
	for (int i = 0; i < static_cast<int>(ELogLevel::Count); ++i)
	{
		if (m_logFiles[i].is_open())
		{
			m_logFiles[i].close();
		}
	}
}

void CLogger::Start()
{
	m_bRunning = true;
	CreateThread();
}

void CLogger::Stop()
{
	m_bRunning = false;
	::SetEvent(m_hQueueEvent);
	DestroyThread();
}

void CLogger::Info(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	m_pInstance->EnqueueLog(ELogLevel::Info, format, args);
	va_end(args);
}

void CLogger::Error(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	m_pInstance->EnqueueLog(ELogLevel::Error, format, args);
	va_end(args);
}

void CLogger::Warning(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	m_pInstance->EnqueueLog(ELogLevel::Warning, format, args);
	va_end(args);
}

void CLogger::EnqueueLog(ELogLevel eLevel, const char* format, va_list args)
{
	char buffer[MAX_LOG_BUFFER_SIZE];
	int len = vsnprintf(buffer, MAX_LOG_BUFFER_SIZE, format, args);
	if (len < 0)
	{
		return;
	}
	
	SLogEntry entry;
	{
		entry.eLevel = eLevel;
		entry.strMessage.assign(buffer, (len < MAX_LOG_BUFFER_SIZE) ? len : MAX_LOG_BUFFER_SIZE - 1);
		::GetLocalTime(&entry.stTime);
	}
	
	AUTO_LOCK(m_lock)
	{
		m_vecWriteBuffer.push_back(std::move(entry));
	}
	
	::SetEvent(m_hQueueEvent);
}

DWORD CLogger::FuncImplRun()
{
	while (m_bRunning)
	{
		::WaitForSingleObject(m_hQueueEvent, INFINITE);
		ProcessQueue();
	}
	
	ProcessQueue();
	
	return 0;
}

bool CLogger::ProcessQueue()
{
	AUTO_LOCK(m_lock)
	{
		if (m_vecWriteBuffer.empty())
		{
			return false;
		}
		m_vecReadBuffer.swap(m_vecWriteBuffer);
	}
	
	for (const SLogEntry& entry : m_vecReadBuffer)
	{
		WriteLog(entry);
	}
	
	m_vecReadBuffer.clear();
	return true;
}

void CLogger::WriteLog(const SLogEntry& entry)
{
	int nLevelIndex = static_cast<int>(entry.eLevel);
	
	if (m_wCurrentDay[nLevelIndex] != entry.stTime.wDay)
	{
		OpenLogFile(entry.eLevel, entry.stTime);
	}
	
	std::string strTimestamp = FormatTimestamp(entry.stTime);
	const char* szPrefix = GetLogLevelName(entry.eLevel);
	
	char outputBuffer[MAX_LOG_BUFFER_SIZE + 64];
	int outputLen = snprintf(outputBuffer, sizeof(outputBuffer), 
		"%s|[%s] %s\n", strTimestamp.c_str(), szPrefix, entry.strMessage.c_str());
	
	if (outputLen > 0)
	{
		if (m_logFiles[nLevelIndex].is_open())
		{
			m_logFiles[nLevelIndex].write(outputBuffer, outputLen);
			m_logFiles[nLevelIndex].flush();
		}
		
		std::cout.write(outputBuffer, outputLen);
	}
}

void CLogger::OpenLogFile(ELogLevel eLevel, const SYSTEMTIME& st)
{
	int nLevelIndex = static_cast<int>(eLevel);
	
	if (m_logFiles[nLevelIndex].is_open())
	{
		m_logFiles[nLevelIndex].close();
	}
	
	std::string strFileName = GetLogFileName(eLevel, st);
	m_logFiles[nLevelIndex].open(strFileName, std::ios::app);
	
	m_wCurrentDay[nLevelIndex] = st.wDay;
}

std::string CLogger::FormatTimestamp(const SYSTEMTIME& st)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	return std::string(buffer);
}

std::string CLogger::GetLogFileName(ELogLevel eLevel, const SYSTEMTIME& st)
{
	const char* szLevelName = LOG_LEVEL_NAME_TABLE[static_cast<int>(eLevel)];
	
	char szProcessPath[MAX_PATH] = {};
	::GetModuleFileNameA(NULL, szProcessPath, MAX_PATH);
	char* szProcessName = strrchr(szProcessPath, '\\');
	if (szProcessName != nullptr)
	{
		szProcessName++;
	}
	else
	{
		szProcessName = szProcessPath;
	}
	char* szExt = strrchr(szProcessName, '.');
	if (szExt != nullptr)
	{
		*szExt = '\0';
	}
	
	char buffer[512];
	snprintf(buffer, sizeof(buffer), "%s_%s_%04d%02d%02d.log",
		szLevelName, szProcessName, st.wYear, st.wMonth, st.wDay);
	return std::string(buffer);
}
