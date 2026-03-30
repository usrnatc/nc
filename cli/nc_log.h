#if !defined(__NC_LOG_H__)
#define __NC_LOG_H__

#include "nc_types.h"
#include "nc_string.h"

// NORMAL  "\x1B[0m"
// RED     "\x1B[31m"
// GREEN   "\x1B[32m"
// YELLOW  "\x1B[33m"
// BLUE    "\x1B[34m"
// MAGENTA "\x1B[35m"
// CYAN    "\x1B[36m"
// WHITE   "\x1B[37m"
// RESET   "\033[0m"

// @defines____________________________________________________________________
typedef u8 LogLevel;
enum : u8 {
    LOG_LEVEL_NORMAL,
    LOG_LEVEL_SUCCESS,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_PANIC,
    LOG_LEVEL_COUNT
};

#if defined(NC_DEBUG)
    #define LogNormal(...)  __LogBase(__FILE__, __LINE__, LOG_LEVEL_NORMAL,  __VA_ARGS__)
    #define LogSuccess(...) __LogBase(__FILE__, __LINE__, LOG_LEVEL_SUCCESS, __VA_ARGS__)
    #define LogInfo(...)    __LogBase(__FILE__, __LINE__, LOG_LEVEL_INFO,    __VA_ARGS__)
    #define LogWarn(...)    __LogBase(__FILE__, __LINE__, LOG_LEVEL_WARN,    __VA_ARGS__)
    #define LogError(...)   __LogBase(__FILE__, __LINE__, LOG_LEVEL_ERROR,   __VA_ARGS__)
    #define LogPanic(...)   __LogBase(__FILE__, __LINE__, LOG_LEVEL_PANIC,   __VA_ARGS__)
#else
    #define LogNormal(...)
    #define LogSuccess(...)
    #define LogInfo(...)
    #define LogWarn(...)
    #define LogError(...)
    #define LogPanic(...)
#endif

// @types______________________________________________________________________


// @runtime____________________________________________________________________


// @functions__________________________________________________________________
void __LogBase(char const* File, int Line, u8 Severity, Str8 Msg);
void __LogBase(char const* File, int Line, u8 Severity, char* Fmt, ...);
void __LogBase(char const* File, int Line, u8 Severity, Str16 Msg);
void __LogBase(char const* File, int Line, u8 Severity, wchar* Fmt, ...);

#endif // __NC_LOG_H__
