#pragma once

enum { _4k = 4096 }; // 4kB

#ifdef _DEBUG

#ifdef _WIN64
#define ASSERT(expr, ...) if(!(expr)) __debugbreak();

#elif  _WIN32
#define ASSERT(expr, ...) if(!(expr)) __asm{ int 3 }

#else
#define ASSERT(expr, ...) 
#endif // _WIN64

#else
#define ASSERT(expr, ...) ;
#endif


#define GOTO_BEGIN do {
#define GOTO_END } while(0);
#define GOTO_OUT break

#define DEBUGBREAK ASSERT(false)