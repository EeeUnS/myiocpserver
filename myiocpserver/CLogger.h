#pragma once
#include "CSingleton.h"
#include "CThread.h"

class CLogger : public CSingleton<CLogger>, CThread
{
public:
	CLogger(const CLogger&) = delete;
	CLogger& operator=(const CLogger&) = delete;
	
	static void Info(const char* format, ...);
	static void Error(const char* format, ...);
	static void Warning(const char* format, ...);
};