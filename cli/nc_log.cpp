#include <stdarg.h>

#include "nc_log.h"
#include "nc_print.h"
#include "nc_arena.h"

char const* MessageFormats[LOG_LEVEL_COUNT] = {
    "\x1B[0m    MSG | %S | %-4d | %S\033[0m\n",
    "\x1B[32mSUCCESS | %S | %-4d | %S\033[0m\n",
    "\x1B[36m   INFO | %S | %-4d | %S\033[0m\n",
    "\x1B[34m   WARN | %S | %-4d | %S\033[0m\n",
    "\x1B[33m  ERROR | %S | %-4d | %S\033[0m\n",
    "\x1B[31m  PANIC | %S | %-4d | %S\033[0m\n"
};

void
__LogBase(char const* File, int Line, u8 Severity, Str8 Msg)
{
    Severity = MIN(Severity, LOG_LEVEL_COUNT - 1);
    Severity = MAX(Severity, 0);

    TempArena Scratch = GetScratch(NULL, 0);
    Str8 FilePath = StrSkipLastSlash(CStr8((char*) File));
    Str8 FormattedMsg = ArenaPushStrFmt(
        Scratch.MemPool,
        (char*) MessageFormats[Severity],
        PRINT_STR(FilePath),
        Line,
        PRINT_STR(Msg)
    );

    PrintErr(FormattedMsg);
    ReleaseScratch(Scratch);
}

void 
__LogBase(char const* File, int Line, u8 Severity, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 Msg = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    __LogBase(File, Line, Severity, Msg);
    va_end(Args);
    ReleaseScratch(Scratch);
}

void 
__LogBase(char const* File, int Line, u8 Severity, Str16 Msg)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str8 Msg8 = Str8FromStr16(Scratch.MemPool, Msg);

    __LogBase(File, Line, Severity, Msg8);

    ReleaseScratch(Scratch);
}

void 
__LogBase(char const* File, int Line, u8 Severity, wchar* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str16 Msg = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);
    __LogBase(File, Line, Severity, Msg);
    ReleaseScratch(Scratch);
}
