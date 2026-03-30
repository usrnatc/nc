#include "linux_platform.h"

void 
TimeInit(void)
{}

INTERNAL DateTime
TimeTMToDateTime(tm TM, u32 MSecs)
{
    DateTime Result = {};

    Result.Second = TM.tm_sec;
    Result.Minute = TM.tm_min;
    Result.Hour = TM.tm_hour;
    Result.Day = TM.tm_mday - 1;
    Result.Month = TM.tm_mon;
    Result.Year = TM.tm_year + 1900;
    Result.MSec = MSecs;

    return Result;
}

INTERNAL tm
TimeDateTimeToTM(DateTime Time)
{
    tm Result = {};

    Result.tm_sec = Time.Second;
    Result.tm_min = Time.Minute;
    Result.tm_hour = Time.Hour;
    Result.tm_mday = Time.Day + 1;
    Result.tm_mon = Time.Month;
    Result.tm_year = Time.Year - 1900;

    return Result;
}

INTERNAL timespec
TimeDateTimeToTimeSpec(DateTime Time)
{
    tm TM = TimeDateTimeToTM(Time);
    time_t Seconds = timegm(&TM);
    timespec Result = {};

    Result.tv_sec = Seconds;

    return Result;
}

INTERNAL WallClockTime
TimeTimeSpecToWallClockTime(timespec TimeSpec)
{
    WallClockTime Result = 0;

    {
        tm TM = {};

        gmtime_r(&TimeSpec.tv_sec, &TM);

        DateTime Time = TimeTMToDateTime(TM, TimeSpec.tv_nsec / MILLION(1));
        
        Result = TimeDateTimeToWallClock(&Time);
    }

    return Result;
}

void 
SleepMSecs(u32 MSecs)
{
    usleep(MSecs * THOUSAND(1));
}

PerfCounter 
TimeGetTimestamp(void)
{}

f64 
TimeElapsedSec(PerfCounter Start, PerfCounter End)
{}

WallClockTime 
TimeGetWallClockUTC(void)
{}

void 
TimeWallClockToDateTime(WallClockTime Time, OUT DateTime* DTime)
{}

WallClockTime 
TimeDateTimeToWallClock(DateTime* DTime)
{}
