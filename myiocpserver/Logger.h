#pragma once
#include "Singleton.h"
#include "Thread.h"

class CLogger : public CSingleton<CLogger>, public CThread
{
public:
	CLogger(const CLogger&) = delete;
	CLogger& operator=(const CLogger&) = delete;
	
	static void Info(const char* format, ...);
	static void Error(const char* format, ...);
	static void Warning(const char* format, ...);
}; 