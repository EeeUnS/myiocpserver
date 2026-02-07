#pragma once
#include "Singleton.h"
#include "Thread.h"
#include "Lock.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstdarg>

#define LOG_LEVEL_LIST \
	X(Info) \
	X(Error) \
	X(Warning)

enum class ELogLevel
{
#define X(name) name,
	LOG_LEVEL_LIST
#undef X
	Count
};

struct SLogEntry
{
	ELogLevel eLevel;
	std::string strMessage;
	SYSTEMTIME stTime;
};

class CLogger : public CSingleton<CLogger>, public CThread
{
	friend class CSingleton<CLogger>;
	friend struct std::default_delete<CLogger>;
public:
	CLogger(const CLogger&) = delete;
	CLogger& operator=(const CLogger&) = delete;
	
	static void Info(const char* format, ...);
	static void Error(const char* format, ...);
	static void Warning(const char* format, ...);

	void Start();
	void Stop();

protected:
	virtual DWORD FuncImplRun() override;

private:
	CLogger();
	~CLogger();

	void EnqueueLog(ELogLevel eLevel, const char* format, va_list args);
	void WriteLog(const SLogEntry& entry);
	void OpenLogFile(ELogLevel eLevel, const SYSTEMTIME& st);
	std::string FormatTimestamp(const SYSTEMTIME& st);
	std::string GetLogFileName(ELogLevel eLevel, const SYSTEMTIME& st);

	bool ProcessQueue();

	CLock m_lock;
	std::vector<SLogEntry> m_vecWriteBuffer;
	std::vector<SLogEntry> m_vecReadBuffer;
	HANDLE m_hQueueEvent;
	bool m_bRunning;
	
	std::ofstream m_logFiles[static_cast<int>(ELogLevel::Count)];
	WORD m_wCurrentDay[static_cast<int>(ELogLevel::Count)];
};
