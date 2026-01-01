// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#include <Windows.h>


#include <mswsock.h>
#include <process.h>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#include "DebugHelper.h"

#define CACHE_LINE __declspec(align(64))

#endif //PCH_H
